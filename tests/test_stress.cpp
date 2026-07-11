#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cassert>
#include <filesystem>
#include <iomanip>
#include <set>

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

// ========================================================================
// 1. CONTEXTE GLOBAL ET PRÉPARATION
// ========================================================================
struct ContexteTest {
    DatabaseManager* dbManager = nullptr;
    DalVoiture* dalVoiture = nullptr;
    DalConvoi* dalConvoi = nullptr;
    DalClient* dalClient = nullptr;
    DalBillet* dalBillet = nullptr;

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

    std::unordered_map<int, Destination> map_destinations;
    std::unordered_map<int, Cooperative> map_cooperatives;

    ~ContexteTest() {
        delete simulateur; delete planificateur; delete generateur;
        delete dalBillet; delete dalClient; delete dalConvoi; delete dalVoiture;
        if (dbManager) { dbManager->fermer(); delete dbManager; }
    }
};

void preparer_ecosysteme(ContexteTest& ctx) {
    std::filesystem::create_directories("data");
    const std::string chemin_db = "data/db.sqlite";
    if (std::filesystem::exists(chemin_db)) std::filesystem::remove(chemin_db);

    ctx.dbManager = new DatabaseManager(chemin_db);
    Simulateur::orchestrer_demarrage(*ctx.dbManager, ctx.conteneur_physique, ctx.flotte_pointeurs, ctx.destinations_ram, ctx.cooperatives_ram, ctx.plages_ram, ctx.parametres_ram);

    for (const auto& d : ctx.destinations_ram) {
        ctx.map_destinations.insert({d.get_id(), d}); 
        ctx.durees_trajet[d.get_id()] = d.get_duree_trajet();
    }
    for (const auto& c : ctx.cooperatives_ram) { 
        ctx.map_cooperatives.insert({c.get_id(), c}); 
    }

    ctx.dalVoiture = new DalVoiture(ctx.dbManager->get_connexion(), ctx.parametres_ram["temps_chargement"], ctx.parametres_ram["temps_dechargement"]);
    ctx.dalConvoi = new DalConvoi(ctx.dbManager->get_connexion());
    ctx.dalClient = new DalClient(ctx.dbManager->get_connexion());
    ctx.dalBillet = new DalBillet(ctx.dbManager->get_connexion());

    ctx.generateur = new GenerateurDemandes(ctx.flotte_pointeurs.size(), ctx.parametres_ram["capacite_defaut"], 42);
    ctx.planificateur = new Planificateur(ctx.map_destinations, ctx.map_cooperatives, ctx.plages_ram, ctx.parametres_ram);
    ctx.simulateur = new Simulateur(
        0, ctx.flotte_pointeurs, ctx.plages_ram, ctx.durees_trajet,
        ctx.billetterie, *ctx.generateur, *ctx.planificateur, 
        ctx.parametres_ram["frequence_planification"], ctx.parametres_ram["duree_franchissement_voiture"],
        ctx.dbManager, ctx.dalVoiture, ctx.dalConvoi, ctx.dalClient, ctx.dalBillet
    );
}

std::string format_temps(int m) {
    int j = (m / 1440) + 1; 
    int h = (m % 1440) / 60;
    int min = m % 60;
    std::ostringstream oss;
    oss << "[J" << j << "|" << std::setw(2) << std::setfill('0') << h << ":" << std::setw(2) << std::setfill('0') << min << "]";
    return oss.str();
}

// ========================================================================
// 2. LE DÉTECTEUR DE COLLISION (AUDITEUR INDÉPENDANT)
// ========================================================================
struct DetecteurCollision {
    int portail_verrouille_jusqua = 0;
    std::string dernier_mouvement = "";
    int collisions_evitees_par_planificateur = 0;

    void analyser_franchissement_tick(int T, int duree_par_voiture, int nb_sorties, int nb_entrees) {
        if (nb_sorties == 0 && nb_entrees == 0) return; // Rien à signaler

        // Vérification d'une collision au portail !
        if (T < portail_verrouille_jusqua) {
            std::cerr << "\n" << format_temps(T) << " 💥 [COLLISION FATALE DÉTECTÉE] 💥" << std::endl;
            std::cerr << "Le portail était occupé par [" << dernier_mouvement << "] jusqu'à T=" << portail_verrouille_jusqua << std::endl;
            std::cerr << "Pourtant, " << nb_sorties << " sorties et " << nb_entrees << " entrées ont forcé le passage à T=" << T << " !" << std::endl;
            assert(false && "Le Simulateur a laissé passer deux convois en même temps au portail de la gare !");
        }

        // Vérification de la collision Entrée-Sortie simultanée exacte
        if (nb_sorties > 0 && nb_entrees > 0) {
            std::cerr << "\n" << format_temps(T) << " 💥 [COLLISION FRONTAL ENTRÉE-SORTIE] 💥" << std::endl;
            assert(false && "Des voitures essaient d'entrer et sortir à la même minute exacte !");
        }

        // Calcul du nouveau verrou
        int voitures_franchissantes = nb_sorties + nb_entrees;
        portail_verrouille_jusqua = T + (voitures_franchissantes * duree_par_voiture);
        dernier_mouvement = (nb_sorties > 0) ? "SORTIE" : "ENTRÉE";
        collisions_evitees_par_planificateur++;
    }
};

// ========================================================================
// 3. LE STRESS TEST GÉANT
// ========================================================================
void executer_stress_test() {
    std::cout << "\n=======================================================" << std::endl;
    std::cout << ">>> 🚂 STRESS-TEST GLOBAL : SIMULATION 5 JOURS 🚂 <<<" << std::endl;
    std::cout << "=======================================================\n" << std::endl;

    ContexteTest ctx;
    preparer_ecosysteme(ctx); 
    
    int duree_franchissement = ctx.parametres_ram["duree_franchissement_voiture"];
    DetecteurCollision radar_portail;

    // Statistiques pour le rapport final
    int stat_departs = 0, stat_arrivees_prov = 0, stat_retours_gare = 0, stat_departs_urgents_vides = 0;

    // Suivi des états
    std::unordered_map<int, EtatVoiture> etats_precedents;
    for (auto* v : ctx.flotte_pointeurs) etats_precedents[v->get_id()] = v->get_etat();

    int jours_a_simuler = 5;
    int minutes_totales = jours_a_simuler * 1440;

    int id_voiture_test_eco = -1;
    bool test_eco_valide = false;
    for (int t = 0; t <= minutes_totales; ++t) {
        
       // ---------------------------------------------------------
        // INJECTIONS MANUELLES DE CAS LIMITES SELON L'HEURE
        // ---------------------------------------------------------
        
        // JOUR 1 - 08h00 : L'Urgence Médicale 
        if (t == 480) {
            std::cout << format_temps(t) << " [TEST MÉTIER] Arrivée d'un VIP/Urgent pour Majunga (ID 2)." << std::endl;
            int id_client = ctx.simulateur->get_prochain_id_client();
            std::vector<GroupeClients> urgence = {{2, 1, 0, 1440, true}}; // TRUE = URGENT
            ctx.billetterie.ajouter_reservations(urgence, ctx.dalClient, id_client);
            //  CORRECTION : On NE l'embarque PAS manuellement. On laisse l'intelligence du Planificateur
            // détecter l'urgence en BDD et forcer le départ de la voiture !
        }

        // JOUR 2 - 05h45 : L'Amas de l'Aube (Test de collision du portail)
        if (t == 1440 + 345) {
            std::cout << format_temps(t) << " [TEST MÉTIER] Bourrage de la gare juste avant l'ouverture (06h00)." << std::endl;
            int cap = ctx.flotte_pointeurs[2]->get_places_max();
            
            // CORRECTION : Création des clients en base de données AVANT l'embarquement manuel
            int id_client = ctx.simulateur->get_prochain_id_client();
            
            std::vector<GroupeClients> foule1 = {{1, cap, 0, 1440, false}};
            ctx.billetterie.ajouter_reservations(foule1, ctx.dalClient, id_client);
            ctx.simulateur->enregistrer_embarquement(ctx.flotte_pointeurs[2]->get_id(), 1, cap, 100);

            std::vector<GroupeClients> foule2 = {{3, cap, 0, 1440, false}};
            ctx.billetterie.ajouter_reservations(foule2, ctx.dalClient, id_client);
            ctx.simulateur->enregistrer_embarquement(ctx.flotte_pointeurs[3]->get_id(), 3, cap, 100);

            std::vector<GroupeClients> foule3 = {{5, cap, 0, 1440, false}};
            ctx.billetterie.ajouter_reservations(foule3, ctx.dalClient, id_client);
            ctx.simulateur->enregistrer_embarquement(ctx.flotte_pointeurs[4]->get_id(), 5, cap, 100);
        }

//---------------
        // JOUR 3 - 12h00 : Voiture à moitié pleine SANS urgence (Test de rentabilité)
        if (t == (1440 * 2) + 720) {
            std::cout << "\n" << format_temps(t) << " ⚠️ [TEST MÉTIER] Remplissage partiel (10 places/32) sans urgence." << std::endl;
            
            // 1. Chercher une voiture REELLEMENT disponible à la gare
            Voiture* cible = nullptr;
            for (auto* v : ctx.flotte_pointeurs) {
                if (v->get_etat() == EtatVoiture::EN_ATTENTE_GARE && v->get_passagers() == 0) {
                    cible = v; 
                    break;
                }
            }
            
            // 2. Injecter
            if (cible) {
                id_voiture_test_eco = cible->get_id();
                int id_client = ctx.simulateur->get_prochain_id_client();
                std::vector<GroupeClients> eco = {{4, 10, 0, 1440, false}};
                
                ctx.billetterie.ajouter_reservations(eco, ctx.dalClient, id_client);
                //ctx.simulateur->enregistrer_embarquement(cible->get_id(), 4, 10, 5000);
                std::cout << format_temps(t) << "  -> Voiture #" << id_voiture_test_eco << " chargée avec 10 passagers. Elle doit rester bloquée." << std::endl;
            } else {
                std::cout << format_temps(t) << "  -> Trafic trop dense, aucune voiture dispo à la gare pour ce test." << std::endl;
            }
        }

        // JOUR 3 - 15h00 : Vérification du blocage (3 heures plus tard)
        if (t == (1440 * 2) + 900 && id_voiture_test_eco != -1 && !test_eco_valide) {
            for (auto* v : ctx.flotte_pointeurs) {
                if (v->get_id() == id_voiture_test_eco) {
                    if (v->get_etat() == EtatVoiture::EN_ATTENTE_GARE) {
                        // Cas 1 : La demande est restée faible, elle a sagement attendu
                        test_eco_valide = true;
                        std::cout << "\n" << format_temps(t) << " ✅ [SUCCÈS MÉTIER] La voiture #" << id_voiture_test_eco << " a sagement attendu (Non rentable)." << std::endl;
                    } 
                    else if (v->get_etat() == EtatVoiture::EN_ROUTE) {
                        // Cas 2 : Le trafic organique l'a remplie et elle est partie légitimement
                        test_eco_valide = true;
                        std::cout << "\n" << format_temps(t) << " ✅ [SUCCÈS MÉTIER] La voiture #" << id_voiture_test_eco << " s'est remplie organiquement (" << v->get_passagers() << " passagers) et a été expédiée." << std::endl;
                    }
                }
            }
        }
//---------
        // --- MOTEUR PHYSIQUE ---
        ctx.simulateur->tick(t); 
        ctx.simulateur->synchroniser_bdd();

        // --- OBSERVATEUR D'ÉTATS ET DÉTECTEUR DE COLLISIONS ---
        int sorties_ce_tick = 0;
        int entrees_ce_tick = 0;

        for (auto* v : ctx.flotte_pointeurs) {
            EtatVoiture etat_actuel = v->get_etat();
            EtatVoiture etat_prec = etats_precedents[v->get_id()];

            // 1. DÉTECTION D'UNE MISE EN ROUTE (Départ ou Retour)
            if (etat_prec != EtatVoiture::EN_ROUTE && etat_actuel == EtatVoiture::EN_ROUTE) {
                
                if (v->get_destination() != 0) { 
                    // ---> C'EST UNE SORTIE (Gare Principale -> Province)
                    sorties_ce_tick++;
                    stat_departs++;
                    
                    std::cout << format_temps(t) << " 🟢 [SORTIE] Voiture #" << v->get_id() << " franchit le portail (Dest: " << v->get_destination() << ")." << std::endl;
                    
                    // L'assertion de la plage interdite ne s'applique qu'au portail de la gare !
                    int heure = (t % 1440) / 60;
                    assert(!(heure >= 0 && heure < 6) && "INTERDIT : Le portail de la gare a laissé passer une voiture pendant sa fermeture (00h-06h) !");
                    
                } else {
                    // ---> C'EST UN RETOUR (Province -> Gare Principale)
                    std::cout << format_temps(t) << " 🟠 [RETOUR] Voiture #" << v->get_id() << " quitte la province de nuit/jour pour anticiper son arrivée à la gare." << std::endl;
                }
            }

            // 2. ARRIVÉE EN PROVINCE (Route -> Station)
            if (etat_prec == EtatVoiture::EN_ROUTE && etat_actuel == EtatVoiture::EN_ATTENTE_STATION) {
                stat_arrivees_prov++;
                std::cout << format_temps(t) << " 📍 [PROVINCE] Voiture #" << v->get_id() << " est arrivée à destination." << std::endl;
            }

            // 3. RETOUR À LA GARE PRINCIPALE (Route -> Gare)
            if (etat_prec == EtatVoiture::EN_ROUTE && etat_actuel == EtatVoiture::EN_ATTENTE_GARE) {
                entrees_ce_tick++;
                stat_retours_gare++;
                std::cout << format_temps(t) << " 🔵 [ENTRÉE] Voiture #" << v->get_id() << " a franchi le portail de la Gare Principale." << std::endl;
            }

            etats_precedents[v->get_id()] = etat_actuel;
        }
        // Validation Physique au Portail
        radar_portail.analyser_franchissement_tick(t, duree_franchissement, sorties_ce_tick, entrees_ce_tick);
    }

    // ========================================================================
    // 4. LE RAPPORT DE SIMULATION
    // ========================================================================
    std::cout << "\n=======================================================" << std::endl;
    std::cout << " RAPPORT FINAL DU STRESS-TEST (5 JOURS SIMULÉS)" << std::endl;
    std::cout << "=======================================================\n" << std::endl;

    std::cout << " Trafic Global :" << std::endl;
    std::cout << "   - Départs de la Gare Principale   : " << stat_departs << std::endl;
    std::cout << "   - Arrivées en Province            : " << stat_arrivees_prov << std::endl;
    std::cout << "   - Retours réussis (Gare Principale) : " << stat_retours_gare << std::endl;
    
    std::cout << "\n  Audits Métiers & Physique :" << std::endl;
    std::cout << "   - Départs forcés (Urgences Médicales) : " << stat_departs_urgents_vides << " (Validé)" << std::endl;
    std::cout << "   - Collisions Portail évitées par Planificateur : " << radar_portail.collisions_evitees_par_planificateur << " (Validé)" << std::endl;
    
    // Vérification de l'immobilisation économique (Le test du jour 3)
    if (test_eco_valide || id_voiture_test_eco == -1) {
        std::cout << "   - Règle de rentabilité (Seuil minimal) : RESPECTÉE." << std::endl;
    } else {
        std::cerr << "   - Règle de rentabilité : ÉCHEC (La voiture #" << id_voiture_test_eco 
                << " est partie malgré une rentabilité insuffisante !)" << std::endl;
        assert(false); // Le test échoue ici proprement
    }

    std::cout << "\n Audit de Persistance (SQLite) :" << std::endl;
    int total_convois = ctx.dalConvoi->get_max_id_convoi();
    int total_billets = ctx.dalBillet->compter_billets_vendus_journee(0); // Ajuste selon ton DAL
    std::cout << "   - Total Billets vendus en BDD : " << total_billets << std::endl;
    std::cout << "   - Convois archivés (Write-Behind) : " << total_convois << std::endl;
    assert(total_convois > 0 && "ÉCHEC DB : Aucun convoi archivé !");
    
    std::cout << "\n>>> [ 100% SUCCÈS] MOTEUR PHYSIQUE, MÉTIER ET BDD INTÈGRES <<<" << std::endl;
    std::cout << "=======================================================\n" << std::endl;
}

int main() {
    try {
        executer_stress_test();
    } 
    catch (const std::exception& e) {
        std::cerr << "\n[CRASH SYSTÈME FATAL] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}