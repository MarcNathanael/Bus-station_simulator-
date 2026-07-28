#include <iostream>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <filesystem>
#include "../core/Configuration.h"
#include "../core/Destination.h"
#include "../simulateur/Simulateur.h"
#include "../simulateur/Planificateur.h"
#include "../simulateur/Billetterie.h"
#include "../simulateur/Generateur.h"
#include "../db/DatabaseManager.h"
// test_stress avec gpu
void executer_test_post_livraison() {
    std::cout << "\n=======================================================" << std::endl;
    std::cout << ">>> TEST POST-LIVRAISON (Backend UI-Ready) <<<" << std::endl;
    std::cout << "=======================================================\n" << std::endl;

    // 1. AMORÇAGE
    std::filesystem::create_directories("data");
    const std::string chemin_db = "data/db_test.sqlite";
    if (std::filesystem::exists(chemin_db)) std::filesystem::remove(chemin_db);

    DatabaseManager dbManager(chemin_db);
    std::vector<Voiture> conteneur_physique;
    std::vector<Voiture*> flotte_pointeurs;
    std::vector<Destination> destinations_ram;
    std::vector<Cooperative> cooperatives_ram;
    std::vector<PlageInterdite> plages_ram;
    std::unordered_map<std::string, int> parametres_ram;
    std::unordered_map<int, int> durees_trajet;

    assert(Simulateur::orchestrer_demarrage(dbManager, conteneur_physique, flotte_pointeurs, destinations_ram, cooperatives_ram, plages_ram, parametres_ram));
    for (const auto& d : destinations_ram) durees_trajet[d.get_id()] = d.get_duree_trajet();

    Billetterie billetterie;
    GenerateurDemandes generateur(flotte_pointeurs.size(), parametres_ram["capacite_defaut"], 42);
    // Construction manuelle de la map des destinations
    std::unordered_map<int, Destination> map_destinations;
    for (const auto& d : destinations_ram) {
        map_destinations.emplace(d.get_id(), d); // emplace évite le constructeur par défaut
    }

    // Construction manuelle de la map des cooperatives
    std::unordered_map<int, Cooperative> map_cooperatives;
    for (const auto& c : cooperatives_ram) {
        map_cooperatives.emplace(c.get_id(), c); 
    }

    // passer ces maps au Planificateur :
    Planificateur planificateur(
        map_destinations,
        map_cooperatives,
        plages_ram,
        parametres_ram
    );

    Simulateur simulateur(0, flotte_pointeurs, plages_ram, durees_trajet, billetterie, generateur, planificateur, 15, 2, &dbManager, nullptr, nullptr, nullptr, nullptr);

    // 2. TEST DU PONT TEMPOREL (Phase 1)
    std::cout << "[TEST 1] Pont temporel et controle du temps..." << std::endl;
    assert(simulateur.est_en_pause() == true); // Doit démarrer en pause
    simulateur.set_en_pause(false);
    assert(simulateur.est_en_pause() == false);
    
    simulateur.set_vitesse(100);
    assert(simulateur.get_vitesse() == 100);
    
    double temps_initial = simulateur.get_temps_continu();
    for(int i=0; i<10; ++i) simulateur.avancer_dune_minute();
    assert(simulateur.get_temps_continu() == temps_initial + 10);
    std::cout << "  -> SUCCES : Temps avance correctement (10 min) et vitesse ajustable." << std::endl;

    // 3. TEST DES INJECTIONS (Phase 1)
    std::cout << "[TEST 2] Outils d'injection manuelle..." << std::endl;
    simulateur.injecter_demande_manuelle(2, 50, false, true); // 50 urgents vers Majunga
    int std_attente = 0, urg_attente = 0;
    billetterie.obtenir_compteurs_attente(simulateur.get_temps_continu(), std_attente, urg_attente);
    assert(urg_attente >= 50); // Doit y avoir au moins nos 50 urgents
    std::cout << "  -> SUCCES : Injection manuelle d'urgences validee (" << urg_attente << " urgents en file)." << std::endl;

    // 4. TEST DU LIVE TUNING (Phase 2)
    std::cout << "[TEST 3] Live Tuning du Planificateur..." << std::endl;
    planificateur.set_taille_max_convoi(3);
    planificateur.set_seuil_remplissage_min(90.0); // 90%
    // On lance une planification pour voir si ça s'applique sans crash
    for(int i=0; i<20; ++i) simulateur.avancer_dune_minute(); // Atteint un cycle de planification
    std::cout << "  -> SUCCES : Parametres modifiables en live sans crash." << std::endl;

    // 5. TEST D'AJOUT DE PLAGE INTERDITE (Phase 1)
    std::cout << "[TEST 4] Ajout dynamique de plage interdite..." << std::endl;
    int taille_avant = plages_ram.size();
    simulateur.ajouter_plage_interdite_ui(600, 700);
    assert(simulateur.get_plages_interdites().size() == taille_avant + 1);    std::cout << "  -> SUCCES : Plage interdite ajoutee dynamiquement." << std::endl;

    // 6. TEST DES COORDONNEES SPATIALES (Modification préalable)
    std::cout << "[TEST 5] Mapping spatial des destinations..." << std::endl;
    bool a_des_coords = false;
    for (const auto& d : destinations_ram) {
        if (d.get_positionX() != 0.0f && d.get_positionY() != 0.0f) {
            a_des_coords = true;
            break;
        }
    }
    assert(a_des_coords);
    std::cout << "  -> SUCCES : Les coordonnees X/Y sont bien chargees en RAM." << std::endl;

    std::cout << "\n>>> [100% SUCCÈS] TOUTES LES FONCTIONNALITÉS UI-READY SONT INTÈGRES <<<" << std::endl;
}

int main() {
    try {
        executer_test_post_livraison();
    } catch (const std::exception& e) {
        std::cerr << "[ÉCHEC DU TEST] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
