#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cassert>
#include <filesystem>
#include <thread>
#include <chrono>

#include "../src/core/Configuration.h"
#include "../src/core/Voiture.h"
#include "../src/core/Destination.h"
#include "../src/core/Cooperative.h"
#include "../src/core/Convoi.h"
#include "../src/core/PlageInterdite.h"

#include "../src/simulateur/Planificateur.h"
#include "../src/simulateur/Generateur.h"
#include "../src/simulateur/Billetterie.h"
#include "../src/simulateur/Simulateur.h"

#include "../src/db/DatabaseManager.h"
#include "../src/db/dal_voiture.h"
#include "../src/db/dal_convoi.h"
#include "../src/db/dal_client.h"
#include "../src/db/dal_billet.h"
#include "../src/db/dal_configuration.h"

// ========================================================================
// CONTEXTE GLOBAL 
// ========================================================================
struct ContexteTest {
    DatabaseManager* dbManager = nullptr;
    DalVoiture* dalVoiture = nullptr;
    DalConvoi* dalConvoi = nullptr;
    DalClient* dalClient = nullptr;
    DalBillet* dalBillet = nullptr;

    std::vector<Voiture> conteneur_physique;
    std::vector<Voiture*> flotte_pointeurs; // convoi
    std::vector<Destination> destinations_ram;
    std::vector<Cooperative> cooperatives_ram;
    std::vector<PlageInterdite> plages_ram;
    std::unordered_map<std::string, int> parametres_ram;
    std::unordered_map<int, int> durees_trajet;

    Billetterie billetterie;
    GenerateurDemandes* generateur = nullptr;
    Planificateur* planificateur = nullptr;
    Simulateur* simulateur = nullptr;

    ~ContexteTest() {
        delete simulateur;
        delete planificateur;
        delete generateur;
        delete dalBillet;
        delete dalClient;
        delete dalConvoi;
        delete dalVoiture;
        if (dbManager) {
            dbManager->fermer();
            delete dbManager;
        }
    }
};

void preparer_ecosysteme(ContexteTest& ctx) {
    const std::string chemin_db = "data/db.sqlite";
    if (std::filesystem::exists(chemin_db)) {
        std::filesystem::remove(chemin_db);
    }

    ctx.dbManager = new DatabaseManager(chemin_db);
    Simulateur::orchestrer_demarrage(*ctx.dbManager, ctx.conteneur_physique, ctx.flotte_pointeurs, ctx.destinations_ram, ctx.cooperatives_ram, ctx.plages_ram, ctx.parametres_ram);

    //prepare le destination
    std::unordered_map<int, Destination> map_destinations;
    for (const auto& d : ctx.destinations_ram) {
        map_destinations.insert({d.get_id(), d}); 
        ctx.durees_trajet[d.get_id()] = d.get_duree_trajet();
    }
    
    //prepare les cooperative
    std::unordered_map<int, Cooperative> map_cooperatives;
    for (const auto& c : ctx.cooperatives_ram) { 
        map_cooperatives.insert({c.get_id(), c}); 
    }

    ctx.dalVoiture = new DalVoiture(ctx.dbManager->get_connexion(), ctx.parametres_ram["temps_chargement"], ctx.parametres_ram["temps_dechargement"]);
    ctx.dalConvoi = new DalConvoi(ctx.dbManager->get_connexion());
    ctx.dalClient = new DalClient(ctx.dbManager->get_connexion());
    ctx.dalBillet = new DalBillet(ctx.dbManager->get_connexion());

    ctx.generateur = new GenerateurDemandes(ctx.flotte_pointeurs.size(), ctx.parametres_ram["capacite_defaut"], 42);
    ctx.planificateur = new Planificateur(map_destinations, map_cooperatives, ctx.plages_ram, ctx.parametres_ram);

    ctx.simulateur = new Simulateur(
        0, ctx.flotte_pointeurs, ctx.plages_ram, ctx.durees_trajet,
        ctx.billetterie, *ctx.generateur, *ctx.planificateur, 
        ctx.parametres_ram["frequence_planification"], ctx.parametres_ram["duree_franchissement_voiture"],
        ctx.dbManager, ctx.dalVoiture, ctx.dalConvoi, ctx.dalClient, ctx.dalBillet
    );
}

// ========================================================================
// SCÉNARIO PRINCIPAL : CYCLE DE VIE COMPLET D'UN VOYAGE
// ========================================================================
void executer_test_integral_voyage() {
    std::cout << "\n=======================================================" << std::endl;
    std::cout << ">>> DEBUT DU SCENARIO : LE VOYAGE DU CLIENT VIP <<<" << std::endl;
    std::cout << "=======================================================\n" << std::endl;

    ContexteTest ctx;
    preparer_ecosysteme(ctx); 

    // --- SÉLECTION DE LA CIBLE ---
    assert(!ctx.flotte_pointeurs.empty() && "La flotte ne doit pas etre vide.");
    Voiture* voiture_cible = ctx.flotte_pointeurs[0];
    int id_voiture = voiture_cible->get_id();
    int capacite = voiture_cible->get_places_max();
    int id_destination = voiture_cible->get_destination();
    int duree_trajet_minutes = ctx.durees_trajet[id_destination];

    std::cout << "[PHASE 1 : PREPARATION]" << std::endl;
    std::cout << "  > Voiture sélectionnée : #" << id_voiture << " (Capacité: " << capacite << " places)" << std::endl;
    std::cout << "  > Destination prévue   : Province #" << id_destination << std::endl;
    std::cout << "  > Durée estimée        : " << duree_trajet_minutes << " minutes\n" << std::endl;

    // --- INJECTION DES CLIENTS POUR REMPLIR LA VOITURE ---
    std::cout << "[PHASE 2 : ARRIVEE DES CLIENTS A LA GARE]" << std::endl;
    // on prend le prochain client 
    int id_client_vip = ctx.simulateur->get_prochain_id_client();
    int id_vip_initial = id_client_vip;

    // On crée un groupe exactement égal à la capacité de la voiture
    std::vector<GroupeClients> flux_test = {
        {id_destination, capacite, 0, 1440, false} 
    };
    
    //le client creer 
    ctx.billetterie.ajouter_reservations(flux_test, ctx.dalClient, id_client_vip);
    
    std::vector<Client> attente_db = ctx.dalClient->charger_tout();
    assert(attente_db.size() == static_cast<size_t>(capacite));
    std::cout << "  > [OK] " << capacite << " clients créés en BDD. Le VIP est le #" << id_client_vip << ".\n" << std::endl;

    // --- EMBARQUEMENT ---
    std::cout << "[PHASE 3 : EMBARQUEMENT]" << std::endl;
    ctx.simulateur->enregistrer_embarquement(id_voiture, id_destination, capacite, 15000.0);
    ctx.simulateur->synchroniser_bdd(); // Forcer la sauvegarde
    
    assert(voiture_cible->est_pleine() == true);
    assert(ctx.dalClient->charger_tout().empty() == true); // Plus personne en attente
    assert(ctx.dalBillet->compter_billets_vendus_journee(0) == capacite);

    std::cout << "  > [OK] Voiture " << id_voiture << " totalement remplie." << std::endl;
    std::cout << "  > [OK] Le client VIP possède son billet." << std::endl;
    std::cout << "  > [OK] Synchronisation BDD effectuée (Write-behind).\n" << std::endl;
    std::cout << "  > [OK] " << capacite << " clients créés. Le VIP est le #" << id_vip_initial << ".\n";

    // --- LE VOYAGE (BOUCLE TEMPORELLE) ---
    std::cout << "[PHASE 4 : LE VOYAGE (SIMULATION TEMPORELLE)]" << std::endl;
    
    bool a_pris_la_route = false;
    bool est_arrive = false;
    int marge_securite = duree_trajet_minutes + 400; // On s'accorde marge max

    for (int t = 0; t <= marge_securite; ++t) {
        
        // 1. On avance le temps d'une minute
        ctx.simulateur->tick(t); 

        // 2. On sauvegarde en BDD les changements induits par ce tick
        ctx.simulateur->synchroniser_bdd();

        EtatVoiture etat_actuel = voiture_cible->get_etat();

        if (etat_actuel == EtatVoiture::EN_ROUTE && !a_pris_la_route) {
            std::cout << "  > [T = " << t << " min] DEPART ! La voiture quitte la gare principale." << std::endl;
            a_pris_la_route = true;
        }

        if (etat_actuel == EtatVoiture::EN_ATTENTE_STATION && a_pris_la_route) {
            std::cout << "  > [T = " << t << " min] ARRIVEE ! La voiture est arrivée dans la province #" << id_destination << "." << std::endl;
            est_arrive = true;
            break; // Le trajet est fini, on stoppe la boucle
        }
    }

    assert(a_pris_la_route && "ERREUR CRITIQUE: La voiture n'est jamais partie de la gare !");
    assert(est_arrive && "ERREUR CRITIQUE: La voiture n'est jamais arrivée à sa destination !");
    std::cout << "\n[PHASE 5 : DEBARQUEMENT ET VALIDATION]" << std::endl;

    // --- DEBARQUEMENT ---
    voiture_cible->debarquer_tous(); 
    ctx.simulateur->synchroniser_bdd();

    assert(voiture_cible->get_passagers() == 0);
    std::cout << "  > [OK] Le client VIP a débarqué avec succès à destination." << std::endl;
    std::cout << "  > [OK] La voiture #" << id_voiture << " est de nouveau vide (0 passagers)." << std::endl;
    
    // Vérification de l'archivage du convoi
    int max_id_convoi = ctx.dalConvoi->get_max_id_convoi();
    assert(max_id_convoi > 0);
    std::cout << "  > [OK] Le trajet a bien été archivé en base de données (Table des Convois)." << std::endl;

    std::cout << "\n=======================================================" << std::endl;
    std::cout << ">>> [SUCCES] L'ECOSYSTEME EST PARFAITEMENT FONCTIONNEL <<<" << std::endl;
    std::cout << "=======================================================\n" << std::endl;
}

// ========================================================================
// MAIN VERBEUX
// ========================================================================
int main() {
    try {
        executer_test_integral_voyage();
    } 
    catch (const std::exception& e) {
        std::cerr << "\n[ECHEC FATAL - EXCEPTION CATCHEE] " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "\n[ECHEC FATAL - ERREUR INCONNUE]" << std::endl;
        return 1;
    }

    return 0;
}