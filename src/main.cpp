#include <iostream>
#include <filesystem>
#include <vector>
#include "ui/Application.h"
#include "simulateur/Simulateur.h"
#include "core/Destination.h"
#include "core/Cooperative.h"
#include "db/DatabaseManager.h"

std::vector<Voiture> conteneur_physique;
std::vector<Voiture*> flotte_pointeurs;
std::vector<Destination> destinations_ram;
std::vector<Cooperative> cooperatives_ram;
std::vector<PlageInterdite> plages_ram;
std::unordered_map<std::string, int> parametres_ram;
std::unordered_map<int, int> durees_trajet;

int main() {
    try {
        std::filesystem::create_directories("data");
        const std::string chemin_db = "data/db.sqlite";

        DatabaseManager dbManager(chemin_db);
        if (!Simulateur::orchestrer_demarrage(dbManager, conteneur_physique, flotte_pointeurs, destinations_ram, cooperatives_ram, plages_ram, parametres_ram)) return 1;

        for (const auto& d : destinations_ram) durees_trajet[d.get_id()] = d.get_duree_trajet();

        std::unordered_map<int, Destination> map_destinations;
        for (const auto& d : destinations_ram) map_destinations.emplace(d.get_id(), d);

        std::unordered_map<int, Cooperative> map_cooperatives;
        for (const auto& c : cooperatives_ram) map_cooperatives.emplace(c.get_id(), c);

        Billetterie billetterie;
        GenerateurDemandes generateur(flotte_pointeurs.size(), parametres_ram["capacite_defaut"], 42);
        
        // FIX DES BUS MANQUANTS : On donne les destinations au générateur !
        for (const auto& d : destinations_ram) {
            if (d.get_id() != 0) {
                generateur.ajouter_destination(d.get_id(), 0.015); // 5.0 = fréquence moyenne
            }
        }

        Planificateur planificateur(map_destinations, map_cooperatives, plages_ram, parametres_ram);

        Simulateur simulateur(0, flotte_pointeurs, plages_ram, durees_trajet, billetterie, generateur, planificateur, parametres_ram["frequence_planification"], parametres_ram["duree_franchissement_voiture"], &dbManager, nullptr, nullptr, nullptr, nullptr);

        Application app(simulateur, destinations_ram);
        app.run();

    } catch (const std::exception& e) {
        std::cerr << "[CRASH] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}