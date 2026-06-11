#include "dal_destination.h"
#include <iostream>

DalDestination::DalDestination(sqlite3* db) : m_db(db) {}

std::vector<Destination> DalDestination::charger_tout() const {
    std::vector<Destination> liste;
    const char* sql = "SELECT id, nom, duree_trajet FROM dal_destinations;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation SELECT Destination : " << sqlite3_errmsg(m_db) << std::endl;
        return liste;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        
        // sqlite3_column_text renvoie du const unsigned char*, on le cast en string
        std::string nom = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int duree = sqlite3_column_int(stmt, 2);

        liste.emplace_back(id, nom, duree);
    }

    sqlite3_finalize(stmt);
    return liste;
}

bool DalDestination::inserer_destination(const Destination& d) {
    const char* sql = "INSERT OR IGNORE INTO dal_destinations (id, nom, duree_trajet) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation INSERT Destination : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, d.get_id());
    sqlite3_bind_text(stmt, 2, d.get_nom().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, d.get_duree_trajet());

    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return succes;
}