#include "dal_voiture.h"
#include <iostream>

DalVoiture::DalVoiture(sqlite3* db) : m_db(db) {}

std::vector<Voiture> DalVoiture::charger_tout() const {
    std::vector<Voiture> flotte;
    const char* sql = "SELECT id, places_max, destination_initiale FROM dal_voitures;"; // Adapte selon ton schéma
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Erreur préparation SELECT : " << sqlite3_errmsg(m_db) << std::endl;
        return flotte;
    }

    // Lecture ligne par ligne
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int places_max = sqlite3_column_int(stmt, 1);
        int dest_init = sqlite3_column_int(stmt, 2);

        // Reconstruction de l'objet métier
        flotte.emplace_back(id, places_max, dest_init);
    }

    sqlite3_finalize(stmt);
    return flotte;
}

bool DalVoiture::mettre_a_jour_voiture(const Voiture& v) {
    // Utilisation des marqueurs (?) pour éviter les injections SQL et faciliter le bind
    const char* sql = "UPDATE dal_voitures SET places_libres = ?, etat = ?, position_actuelle = ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Erreur préparation UPDATE : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    // Association des valeurs aux marqueurs (?)
    sqlite3_bind_int(stmt, 1, v.get_places_libres());
    sqlite3_bind_int(stmt, 2, static_cast<int>(v.get_etat())); // Cast enum en entier
    sqlite3_bind_int(stmt, 3, v.get_position());
    sqlite3_bind_int(stmt, 4, v.get_id());

    // Exécution
    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    
    if (!succes) {
        std::cerr << "Erreur exécution UPDATE : " << sqlite3_errmsg(m_db) << std::endl;
    }

    sqlite3_finalize(stmt); // Toujours libérer la mémoire de la requête
    return succes;
}