#include <iostream>
#include "../core/Configuration.h" // inclut tout le core 

int main() {
    std::cout << "Gare Routiere - Simulateur" << std::endl;
    
    return 0;
}
#include <iostream>
#include <filesystem>
#include "core/Configuration.h"
#include "db/DatabaseManager.h"
#include "db/dal_voiture.h"
#include "db/dal_destination.h"
#include "db/dal_cooperative.h"
#include "db/dal_plage_interdite.h"
#include "db/dal_configuration.h"

// Fonction d'aide pour isoler la logique d'amorçage
bool ammorcer_base_de_donnees(DatabaseManager& dbManager) {
    std::cout << "[Initialisation] Premier lancement : Importation des CSV vers SQLite..." << std::endl;

    // 1. Génération du schéma SQL
    if (!dbManager.executer_script_sql("data/schema.sql")) {
        std::cerr << "Erreur : Impossible d'exécuter schema.sql." << std::endl;
        return false;
    }

    // 2. Parsing des CSV en RAM
    Configuration config_csv;
    if (!config_csv.charger("requirement")) {
        std::cerr << "Erreur : Échec du parsing des fichiers CSV." << std::endl;
        return false;
    }

    // 3. Préparation des DAL
    DalDestination dalDest(dbManager.get_connexion());
    DalCooperative dalCoop(dbManager.get_connexion());
    DalVoiture dalVoit(dbManager.get_connexion());
    DalPlageInterdite dalPlage(dbManager.get_connexion());
    DalConfiguration dalParam(dbManager.get_connexion());

    // 4. Insertion massive dans une transaction unique
    dbManager.commencer_transaction();

    for (const auto& paire : config_csv.get_destinations()) dalDest.inserer_destination(paire.second);
    for (const auto& paire : config_csv.get_cooperatives()) dalCoop.inserer_cooperative(paire.second);
    // Supposons que tu as ajouté inserer_voiture dans DalVoiture
    for (const auto& paire : config_csv.get_voitures()) dalVoit.inserer_voiture(paire.second); 
    for (const auto& plage : config_csv.get_plages()) dalPlage.inserer_plage(plage);
    for (const auto& paire : config_csv.get_parametres()) dalParam.sauvegarder_parametre(paire.first, paire.second);

    dbManager.valider_transaction();

    std::cout << "[Initialisation] Base de données peuplée avec succès !" << std::endl;
    return true;
}

int main() {
    const std::string CHEMIN_DB = "data/db.sqlite";
    bool premier_lancement = !std::filesystem::exists(CHEMIN_DB);

    DatabaseManager dbManager(CHEMIN_DB);
    if (!dbManager.initialiser()) {
        return 1;
    }

    // ÉTAPE 2 : LOGIQUE D'INITIALISATION
    if (premier_lancement) {
        if (!ammorcer_base_de_donnees(dbManager)) {
            return 1; // Arrêt critique si l'importation échoue
        }
    } else {
        std::cout << "[Système] Base de données trouvée. Démarrage normal." << std::endl;
    }

    // ÉTAPE 3 : CHARGEMENT EN MÉMOIRE RAM (CACHE)
    DalVoiture dalVoiture(dbManager.get_connexion());
    std::vector<Voiture> flotte_initiale = dalVoiture.charger_tout();
    
    // Convertir std::vector<Voiture> en std::vector<Voiture*> pour ton Simulateur
    std::vector<Voiture*> flotte_pointeurs;
    for (auto& v : flotte_initiale) {
        flotte_pointeurs.push_back(&v);
    }

    // ... Initialisation du Planificateur, Générateur, Billetterie ...
    
    // On passe la connexion DB (ou les DAL) au simulateur pour qu'il gère le Write-Behind
    /*
    Simulateur simulateur(..., flotte_pointeurs, ..., &dbManager);
    simulateur.executer(1440); // Lancement d'une journée
    */

    return 0;
}