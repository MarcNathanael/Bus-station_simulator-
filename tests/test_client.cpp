#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cassert>
#include <filesystem>

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
// CONTEXTE GLOBAL : Encapsulation pour gérer la mémoire et les dépendances
// ========================================================================
struct ContexteTest {
    DatabaseManager* dbManager = nullptr;
    DalVoiture* dalVoiture = nullptr;
    DalConvoi* dalConvoi = nullptr;
    DalClient* dalClient = nullptr;
    DalBillet* dalBillet = nullptr;

    // Conteneurs mémoires remplis par l'orchestrateur
    std::vector<Voiture> conteneur_physique;
    std::vector<Voiture*> flotte_pointeurs;
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

// ========================================================================
// UTILITAIRE : Initialisation via ton nouvel Orchestrateur
// ========================================================================
ContexteTest preparer_ecosysteme() {
    ContexteTest ctx;
    
    // 1. Nettoyage de la base de données codée en dur dans orchestrer_demarrage
    // pour garantir un environnement de test vierge et reproductible.
    const std::string chemin_db = "data/db.sqlite";
    if (std::filesystem::exists(chemin_db)) {
        std::filesystem::remove(chemin_db);
        std::cout << "[TEST SETUP] Ancienne base supprimee pour test propre." << std::endl;
    }

    ctx.dbManager = new DatabaseManager(chemin_db);

    // 2. Appel de ton orchestrateur statique

    // !!!!! le simulateur a besoin des donner de orchestrer_demarrage dans sas constructeur 
    // car je peux pas creer le simulateur tand que je n'ai pas charger les donner dansla RAM 
    // avec static la methode devient un outils globale de la class :
    /*// ✅ Parfait ! On appelle la fonction directement via la classe.
    // Elle va remplir nos vecteurs et initialiser SQLite.
    bool succes = Simulateur::orchestrer_demarrage(db, conteneur, flotte, ...);

    // Une fois que les vecteurs sont pleins, on peut ENFIN créer le simulateur proprement :
    Simulateur* sim = new Simulateur(0, flotte, ...);*/

    bool succes_demarrage = Simulateur::orchestrer_demarrage(
        *ctx.dbManager, 
        ctx.conteneur_physique, 
        ctx.flotte_pointeurs,
        ctx.destinations_ram, 
        ctx.cooperatives_ram, 
        ctx.plages_ram, 
        ctx.parametres_ram
    );
    assert(succes_demarrage && "L'orchestrateur a echoue au demarrage !");

    // 3. Conversion des vecteurs en maps pour le Planificateur
    std::unordered_map<int, Destination> map_destinations;
    for (const auto& d : ctx.destinations_ram) {
        map_destinations[d.get_id()] = d;
        ctx.durees_trajet[d.get_id()] = d.get_duree_trajet();
    }
    std::unordered_map<int, Cooperative> map_cooperatives;
    for (const auto& c : ctx.cooperatives_ram) {
        map_cooperatives[c.get_id()] = c;
    }

    // 4. Instanciation des DALs pour le test
    ctx.dalVoiture = new DalVoiture(ctx.dbManager->get_connexion(), 
                                    ctx.parametres_ram["temps_chargement"], 
                                    ctx.parametres_ram["temps_dechargement"]);
    ctx.dalConvoi = new DalConvoi(ctx.dbManager->get_connexion());
    ctx.dalClient = new DalClient(ctx.dbManager->get_connexion());
    ctx.dalBillet = new DalBillet(ctx.dbManager->get_connexion());

    // 5. Instanciation des Moteurs
    ctx.generateur = new GenerateurDemandes(ctx.flotte_pointeurs.size(), 40, 42);
    ctx.planificateur = new Planificateur(map_destinations, map_cooperatives, ctx.plages_ram, ctx.parametres_ram);

    // 6. Instanciation du Simulateur final
    ctx.simulateur = new Simulateur(
        0, // Origine
        ctx.flotte_pointeurs, 
        ctx.plages_ram, 
        ctx.durees_trajet,
        ctx.billetterie, 
        *ctx.generateur, 
        *ctx.planificateur, 
        ctx.parametres_ram["frequence_planification"], 
        ctx.parametres_ram["duree_franchissement_voiture"],
        ctx.dbManager,
        ctx.dalVoiture,
        ctx.dalConvoi,
        ctx.dalClient,
        ctx.dalBillet
    );

    return ctx;
}

// ========================================================================
// SCÉNARIO PRINCIPAL : Suivi complet et validation BDD
// ========================================================================
void executer_test_integral() {
    std::cout << "\n[TEST] Demarrage du scenario global..." << std::endl;
    ContexteTest ctx = preparer_ecosysteme();

    // --- ÉTAPE 1 : GÉNÉRATION ET ENREGISTREMENT DU CLIENT ---
    std::cout << "  -> Etape 1 : Injection d'un client anonyme..." << std::endl;
    
    int id_destination_cible = ctx.destinations_ram.empty() ? 1 : ctx.destinations_ram[0].get_id();
    std::vector<GroupeClients> flux_test = {
        {id_destination_cible, 1, 300, 360, false} // 1 passager, t_min=300
    };
    
    // On simule ce que fait le générateur en passant par la billetterie
    // (Note : on suppose qu'un accesseur get_prochain_id_client() existe sur le Simulateur)
    int prochain_id = ctx.simulateur->get_prochain_id_client(); 
    int id_attendu = prochain_id;

    ctx.billetterie.ajouter_reservations(flux_test, ctx.dalClient, prochain_id);

    // Vérification de la création physique en SQLite
    std::vector<Client> attente_db = ctx.dalClient->charger_tout();
    assert(attente_db.size() == 1);
    assert(attente_db[0].get_id() == id_attendu);
    std::cout << "     [OK] Client #" << id_attendu << " insere dans dal_clients_attente (SQLite)." << std::endl;

    // --- ÉTAPE 2 : L'EMBARQUEMENT TRANSACTIONNEL ---
    std::cout << "  -> Etape 2 : L'embarquement (Mutations BDD)..." << std::endl;
    int id_voiture_test = ctx.flotte_pointeurs[0]->get_id();
    
    // Appel de ta méthode d'embarquement corrigée
    ctx.simulateur->enregistrer_embarquement(id_voiture_test, id_destination_cible, 1, 15000.0);

    // Le client doit avoir disparu de la file d'attente
    attente_db = ctx.dalClient->charger_tout();
    assert(attente_db.empty());
    
    // Un billet doit avoir été généré
    int nb_billets = ctx.dalBillet->compter_billets_vendus_journee(0);
    assert(nb_billets == 1);
    std::cout << "     [OK] Client converti en Billet (Transaction Validee)." << std::endl;

    // --- ÉTAPE 3 : WRITE-BEHIND DES VOITURES ---
    std::cout << "  -> Etape 3 : Flag Dirty sur Voiture..." << std::endl;
    Voiture* voiture_test = ctx.flotte_pointeurs[0];
    
    // Modification manuelle (comme si elle partait)
    voiture_test->set_etat(EtatVoiture::EN_ROUTE);
    assert(voiture_test->is_dirty() == true); // Le setter doit avoir levé le flag

    // --- ÉTAPE 4 : WRITE-BEHIND DES CONVOIS ---
    std::cout << "  -> Etape 4 : Flag Dirty sur Convoi et Synchronisation..." << std::endl;
    
    // Utilisation de la nouvelle signature (id, type, taille_max)
    Convoi convoi_test(105, TypeConvoi::SORTIE, 4); 
    convoi_test.ajouter_voiture(voiture_test);
    convoi_test.set_etat(EtatConvoi::TERMINE); // Terminé et modifie = true

    // On l'injecte dans le planificateur
    ctx.planificateur->get_convois_sortie().push_back(convoi_test);

    // Déclenchement de la synchronisation de fin de cycle
    ctx.simulateur->synchroniser_bdd();

    // Vérifications post-synchro
    assert(voiture_test->is_dirty() == false);
    std::cout << "     [OK] Flag dirty nettoye pour la voiture #" << id_voiture_test << "." << std::endl;

    int max_id_convoi = ctx.dalConvoi->get_max_id_convoi();
    assert(max_id_convoi >= 105);
    std::cout << "     [OK] Convoi #105 archive en base (Write-Behind valide)." << std::endl;

    std::cout << "  -> SUCCES : L'ecosysteme complet est hermetique.\n" << std::endl;
}

// ========================================================================
// MAIN
// ========================================================================
int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "   TESTS INTEGRATION : ARCHITECTURE V2     " << std::endl;
    std::cout << "===========================================" << std::endl;

    try {
        executer_test_integral();
        
        std::cout << "\n===========================================" << std::endl;
        std::cout << ">>> [100%] TOUS LES TESTS SONT AU VERT ! <<<" << std::endl;
        std::cout << "===========================================" << std::endl;
    } 
    catch (const std::logic_error& e) {
        // Erreurs liées à l'algorithme (ex: out_of_range, invalid_argument)
        std::cerr << "\n[ECHEC - ERREUR LOGIQUE] " << e.what() << std::endl;
        return 1;
    }
    catch (const std::runtime_error& e) {
        // Erreurs liées à l'environnement (ex: SQLite injoignable, fichier CSV manquant)
        std::cerr << "\n[ECHEC - ERREUR EXECUTION] " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        // Catch global pour le reste des exceptions standards
        std::cerr << "\n[ECHEC - EXCEPTION STANDARD] " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        // Catch absolu pour tout ce qui n'hérite pas de std::exception (ex: un throw int)
        std::cerr << "\n[ECHEC - ERREUR FATALE INCONNUE] Une exception non standard a ete levee." << std::endl;
        return 1;
    }

    return 0;
}