#include "dal_client.h"
#include <iostream>

DalClient::DalClient(sqlite3* db) : m_db(db) {}

std::vector<Client> DalClient::charger_tout() const {
    std::vector<Client> liste;
    const char* sql = "SELECT id, destination_id, t_min, t_max, priorite FROM dal_clients_attente;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation SELECT Client : " << sqlite3_errmsg(m_db) << std::endl;
        return liste;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int dest_id = sqlite3_column_int(stmt, 1);
        int t_min = sqlite3_column_int(stmt, 2);
        int t_max = sqlite3_column_int(stmt, 3);
        
        bool priorite_urgente = (sqlite3_column_int(stmt, 4) == 1);

        // Adapte les paramètres selon le vrai constructeur de ta classe Client
        liste.emplace_back(id, dest_id, t_min, t_max, priorite_urgente);
    }

    sqlite3_finalize(stmt);
    return liste;
}

bool DalClient::inserer_client(const Client& c) {
    const char* sql = "INSERT INTO dal_clients_attente (id, destination_id, t_min, t_max, priorite) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation INSERT Client : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, c.get_id());
    sqlite3_bind_int(stmt, 2, c.get_destination_id());
    sqlite3_bind_int(stmt, 3, c.get_t_min());
    sqlite3_bind_int(stmt, 4, c.get_t_max());
    sqlite3_bind_int(stmt, 5, c.est_urgent() ? 1 : 0);

    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return succes;
}

bool DalClient::supprimer_client(int id_client) {
    const char* sql = "DELETE FROM dal_clients_attente WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation DELETE Client : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, id_client);

    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return succes;
}