#include "dal_plage_interdite.h"
#include <iostream>

DalPlageInterdite::DalPlageInterdite(sqlite3* db) : m_db(db) {}

std::vector<PlageInterdite> DalPlageInterdite::charger_tout() const {
    std::vector<PlageInterdite> liste;
    const char* sql = "SELECT heure_debut, heure_fin FROM dal_plages_interdites;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation SELECT PlageInterdite : " << sqlite3_errmsg(m_db) << std::endl;
        return liste;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int debut = sqlite3_column_int(stmt, 0);
        int fin = sqlite3_column_int(stmt, 1);
        liste.emplace_back(debut, fin);
    }

    sqlite3_finalize(stmt);
    return liste;
}

bool DalPlageInterdite::inserer_plage(const PlageInterdite& plage) {
    const char* sql = "INSERT INTO dal_plages_interdites (heure_debut, heure_fin) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation INSERT PlageInterdite : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, plage.get_debut());
    sqlite3_bind_int(stmt, 2, plage.get_fin());

    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    
    if (!succes) {
        std::cerr << "[DAL] Erreur exécution INSERT PlageInterdite : " << sqlite3_errmsg(m_db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return succes;
}