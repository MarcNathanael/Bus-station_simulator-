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

// pour recuper le client a idee max et l'incrementer pour la prochaine etape 
int DalClient::get_max_id_client() const {
    int max_id = 0;
    
    // Cette requête magique cherche le MAX de l'ID dans les deux tables en même temps
    const char* sql = "SELECT MAX(max_id) FROM ("
                      "  SELECT MAX(id) AS max_id FROM dal_clients_attente "
                      "  UNION "
                      "  SELECT MAX(client_id) AS max_id FROM dal_historique_billets"
                      ");";
    
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) 
    {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            max_id = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    } 
    else 
    {
        std::cerr << "[DAL] Erreur préparation MAX ID Client : " << sqlite3_errmsg(m_db) << std::endl;
    }

    return max_id;
}


std::vector<Client> DalClient::extraire_clients_pour_embarquement(int id_dest, int limite) const {
    std::vector<Client> clients;
    
    // Requête optimisée : On filtre la destination, on trie (les urgents d'abord, puis les plus anciens), et on limite au nombre de places.
    const char* sql = "SELECT id, destination_id, t_min, t_max, priorite "
                      "FROM dal_clients_attente "
                      "WHERE destination_id = ? "
                      "ORDER BY t_min ASC " // LE CORRECTIF EST ICI                      
                      "LIMIT ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id_dest);
        sqlite3_bind_int(stmt, 2, limite);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            clients.emplace_back(
                sqlite3_column_int(stmt, 0),
                sqlite3_column_int(stmt, 1),
                sqlite3_column_int(stmt, 2),
                sqlite3_column_int(stmt, 3),
                (sqlite3_column_int(stmt, 4) == 1)
            );
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "[DAL] Erreur SELECT clients embarquement : " << sqlite3_errmsg(m_db) << std::endl;
    }
    return clients;
}