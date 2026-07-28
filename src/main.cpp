#include <iostream>
#include <filesystem>
#include <vector>
#include "ui/Application.h"
#include "simulateur/Simulateur.h"
#include "core/Destination.h"
#include "core/Cooperative.h"
#include "db/DatabaseManager.h"

// Conteneurs globaux pour la portée du main
std::vector<Voiture> conteneur_physique;
std::vector<Voiture*> flotte_pointeurs;
std::vector<Destination> destinations_ram;
std::vector<Cooperative> cooperatives_ram;
std::vector<PlageInterdite> plages_ram;
std::unordered_map<std::string, int> parametres_ram;
std::unordered_map<int, int> durees_trajet;

int main() {
    try {
        std::cout << "[MAIN] Initialisation de l'environnement..." << std::endl;
        
        std::filesystem::create_directories("data");
        const std::string chemin_db = "data/db.sqlite";
        // On ne supprime plus la BDD pour permettre la persistance entre les lancements
        // if (std::filesystem::exists(chemin_db)) std::filesystem::remove(chemin_db);

        DatabaseManager dbManager(chemin_db);
        if (!Simulateur::orchestrer_demarrage(dbManager, conteneur_physique, flotte_pointeurs, destinations_ram, cooperatives_ram, plages_ram, parametres_ram)) {
            std::cerr << "[ERREUR FATALE] L'orchestration a échoué. Arrêt." << std::endl;
            return 1;
        }

        // Préparation de la map des durées pour le Simulateur
        for (const auto& d : destinations_ram) {
            durees_trajet[d.get_id()] = d.get_duree_trajet();
        }

        // Construction manuelle et robuste des maps pour le Planificateur
        std::unordered_map<int, Destination> map_destinations;
        for (const auto& d : destinations_ram) {
            map_destinations.emplace(d.get_id(), d);
        }

        std::unordered_map<int, Cooperative> map_cooperatives;
        for (const auto& c : cooperatives_ram) {
            map_cooperatives.emplace(c.get_id(), c);
        }

        // Instanciation des composants
        Billetterie billetterie;
        GenerateurDemandes generateur(flotte_pointeurs.size(), parametres_ram["capacite_defaut"], 42);
        Planificateur planificateur(
            map_destinations,
            map_cooperatives,
            plages_ram,
            parametres_ram
        );

        Simulateur simulateur(
            0, flotte_pointeurs, plages_ram, durees_trajet, billetterie, generateur, planificateur,
            parametres_ram["frequence_planification"], parametres_ram["duree_franchissement_voiture"],
            &dbManager, nullptr, nullptr, nullptr, nullptr // Les DALs sont optionnels pour l'UI pure
        );

        std::cout << "[MAIN] Lancement de l'interface graphique..." << std::endl;
        Application app(simulateur, destinations_ram);
        app.run();

    } catch (const std::exception& e) {
        std::cerr << "[CRASH SYSTÈME FATAL] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}