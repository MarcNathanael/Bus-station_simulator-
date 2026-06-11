#include "dal_cooperative.h"
#include <iostream>

DalCooperative::DalCooperative(sqlite3* db) : m_db(db) {}

std::vector<Cooperative> DalCooperative::charger_tout() const {
    std::vector<Cooperative> liste;
    const char* sql = "SELECT id, nom FROM dal_cooperatives;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation SELECT Cooperative : " << sqlite3_errmsg(m_db) << std::endl;
        return liste;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string nom = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        liste.emplace_back(id, nom);
    }

    sqlite3_finalize(stmt);
    return liste;
}

bool DalCooperative::inserer_cooperative(const Cooperative& c) {
    const char* sql = "INSERT OR IGNORE INTO dal_cooperatives (id, nom) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation INSERT Cooperative : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, c.get_id());
    sqlite3_bind_text(stmt, 2, c.get_nom().c_str(), -1, SQLITE_TRANSIENT);

    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return succes;
}