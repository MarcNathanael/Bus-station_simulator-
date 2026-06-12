#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <stdexcept>

class DatabaseManager {
private:
    sqlite3* m_db;
    std::string m_chemin_bdd;

    // Fonction utilitaire privée pour exécuter des requêtes simples sans retour
    bool executer_requete_simple(const std::string& sql);

public:
    explicit DatabaseManager(const std::string& chemin);
    ~DatabaseManager();

    // Gestion de la connexion
    bool initialiser();
    void fermer();

    // Getter pour passer la connexion aux classes DAL
    // les DAL auront un acces read only
    sqlite3* get_connexion() const { return m_db; }

    // Gestion des transactions (Écriture différée / Write-Behind)
    bool commencer_transaction();
    bool valider_transaction();
    bool annuler_transaction(); // Utile en cas d'erreur (ROLLBACK)

    // Exécution de scripts bruts (ex: initialisation du schéma)
    bool executer_script_sql(const std::string& chemin_fichier);
};