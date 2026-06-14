#include "DatabaseManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

DatabaseManager::DatabaseManager(const std::string& chemin) 
    : m_db(nullptr), m_chemin_bdd(chemin) 
{
}

// destructeur , appeler automatiquement a la fin
DatabaseManager::~DatabaseManager() {
    fermer();
}

bool DatabaseManager::executer_requete_simple(const std::string& sql) {
    char* message_erreur = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &message_erreur);
    
    if (rc != SQLITE_OK) {
        std::cerr << "[SQLite Erreur] " << message_erreur << " | Requête: " << sql << std::endl;
        sqlite3_free(message_erreur);
        return false;
    }
    return true;
}

bool DatabaseManager::initialiser() {
    int rc = sqlite3_open(m_chemin_bdd.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        std::cerr << "Impossible d'ouvrir la BDD : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    // Optimisations SQLite pour la performance et la sécurité
    executer_requete_simple("PRAGMA journal_mode = WAL;");
    executer_requete_simple("PRAGMA synchronous = NORMAL;");
    executer_requete_simple("PRAGMA foreign_keys = ON;"); // Toujours activer les clés étrangères

    return true;
}

void DatabaseManager::fermer() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool DatabaseManager::commencer_transaction() {
    return executer_requete_simple("BEGIN TRANSACTION;");
}

bool DatabaseManager::valider_transaction() {
    return executer_requete_simple("COMMIT;");
}

bool DatabaseManager::annuler_transaction() {
    return executer_requete_simple("ROLLBACK;");
}

/*
    utilisation general :
    // On commence le brouillon

    manager.commencer_transaction();
    try {
        // Tu fais tes requêtes d'écriture
        dalVoiture.mettre_a_jour_voiture(v1);
        dalVoiture.mettre_a_jour_voiture(v2);

        // Si on arrive ici sans erreur, on valide !
        manager.valider_transaction();
    } 
    catch (...) {
        // Si le code a planté au milieu, on annule TOUT
        manager.annuler_transaction();
        std::cerr << "Erreur critique, base de données restaurée !" << std::endl;
    }
*/

bool DatabaseManager::executer_script_sql(const std::string& chemin_fichier) {
    std::ifstream fichier(chemin_fichier);
    if (!fichier.is_open()) 
    {
        std::cerr << "Impossible d'ouvrir le fichier SQL : " << chemin_fichier << std::endl;
        return false;
    }

    std::stringstream buffer;
    // comme ici nathan est envoyer dans le flux
    //std::cout << "nathan"; 
    buffer << fichier.rdbuf();
    // on execute les requetes 
    return executer_requete_simple(buffer.str());
}