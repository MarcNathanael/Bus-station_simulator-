#include "dal_destination.h"
#include <iostream>

DalDestination::DalDestination(sqlite3* db) : m_db(db) {}

std::vector<Destination> DalDestination::charger_tout() const {
    std::vector<Destination> destinations;
    const char* sql = "SELECT id, nom, duree_trajet, positionX, positionY FROM dal_destinations;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return destinations;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string nom = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int duree = sqlite3_column_int(stmt, 2);
        float posX = static_cast<float>(sqlite3_column_double(stmt, 3));
        float posY = static_cast<float>(sqlite3_column_double(stmt, 4));
        
        destinations.emplace_back(id, nom, duree, posX, posY);
    }
    
    sqlite3_finalize(stmt);
    return destinations;
}

bool DalDestination::inserer_destination(const Destination& dest) {
    const char* sql = "INSERT OR REPLACE INTO dal_destinations (id, nom, duree_trajet, positionX, positionY) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_int(stmt, 1, dest.get_id());
    sqlite3_bind_text(stmt, 2, dest.get_nom().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, dest.get_duree_trajet());
    sqlite3_bind_double(stmt, 4, static_cast<double>(dest.get_positionX()));
    sqlite3_bind_double(stmt, 5, static_cast<double>(dest.get_positionY()));
    
    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return succes;
}