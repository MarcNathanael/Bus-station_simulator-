#include "dal_billet.h"
#include <iostream>

DalBillet::DalBillet(sqlite3* db) : m_db(db) {}

bool DalBillet::inserer_billet(const Billet& b) {
    // Note: AUTOINCREMENT sur l'ID dans la DB si on ne passe pas d'ID explicite
    const char* sql = "INSERT INTO dal_historique_billets (client_id, voiture_id, heure_depart_min,heure_depart_max, prix) VALUES (?, ?, ?, ?,?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation INSERT Billet : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, b.get_client_id());
    sqlite3_bind_int(stmt, 2, b.get_voiture_id());
    sqlite3_bind_int(stmt, 3, b.get_heure_depart_min());
    sqlite3_bind_int(stmt, 4, b.get_heure_depart_max());
    sqlite3_bind_double(stmt, 5, b.get_prix());

    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return succes;
}

int DalBillet::compter_billets_vendus_journee(int jour_simulation) const {
    int total = 0;
    // On suppose que l'horaire est stocké en minutes continues (1440 min = 1 jour)
    int debut_jour = jour_simulation * 1440;
    int fin_jour = debut_jour + 1440;

    const char* sql = "SELECT COUNT(*) FROM dal_historique_billets WHERE heure_depart >= ? AND heure_depart < ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation COUNT Billets : " << sqlite3_errmsg(m_db) << std::endl;
        return total;
    }

    sqlite3_bind_int(stmt, 1, debut_jour);
    sqlite3_bind_int(stmt, 2, fin_jour);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return total;
}