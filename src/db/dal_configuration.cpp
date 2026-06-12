#include "dal_configuration.h"
#include <iostream>

DalConfiguration::DalConfiguration(sqlite3* db) : m_db(db) {}
std::unordered_map<std::string, int> DalConfiguration::charger_parametres() const 
{
    std::unordered_map<std::string, int> parametres;
    const char* sql = "SELECT cle, valeur FROM dal_parametres;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) 
    {
        std::cerr << "[DAL] Erreur préparation SELECT Configuration : " << sqlite3_errmsg(m_db) << std::endl;
        return parametres;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) //boucle tant qu'il y a des colomn
    {
        // Cast sécurisé du texte SQLite vers std::string
        //change de force le type de ce pointeur sans modifier les données sous-jacentes
        std::string cle = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int valeur = sqlite3_column_int(stmt, 1);

        parametres[cle] = valeur;
    }

    sqlite3_finalize(stmt);
    return parametres;
}

bool DalConfiguration::sauvegarder_parametre(const std::string& cle, int valeur) {
    // Utilisation de INSERT OR REPLACE (équivalent SQLite d'un UPSERT)
    // Si la clé existe déjà (Primary Key), la valeur est mise à jour. Sinon, elle est créée.
    const char* sql = "INSERT OR REPLACE INTO dal_parametres (cle, valeur) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation UPSERT Configuration : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    // Association de la chaîne de caractères (avec SQLITE_TRANSIENT pour la copie sécurisée)
    sqlite3_bind_text(stmt, 1, cle.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, valeur);

    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    
    if (!succes) {
        std::cerr << "[DAL] Erreur exécution UPSERT Configuration : " << sqlite3_errmsg(m_db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return succes;
}