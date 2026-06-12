#include "dal_convoi.h"
#include <iostream>

DalConvoi::DalConvoi(sqlite3* db) : m_db(db) {}

bool DalConvoi::archiver_convoi(const Convoi& c) {
    // 1. Requête SQL mise à jour avec l'état et l'id de la région
    const char* sql = "INSERT INTO dal_historique_convois "
                      "(id_metier, horaire_depart_reel, type_direction, destination_origine_id, "
                      "nb_voitures, contient_urgence, etat_final, id_region) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation ARCHIVE Convoi : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    // Déduction de la direction et de la province cible/origine
    std::string direction = (c.get_type() == TypeConvoi::SORTIE) ? "ALLER" : "RETOUR";
    
    int id_lieu = 0; 
    if (!c.get_voitures().empty() && c.get_voitures().front() != nullptr) {
        id_lieu = (c.get_type() == TypeConvoi::SORTIE) ? 
                  c.get_voitures().front()->get_destination() : 
                  c.get_voitures().front()->get_position();
    }

    // 2. Association rigoureuse des valeurs aux trous '?' (De 1 à 9)
    sqlite3_bind_int(stmt, 1, c.get_id());
    sqlite3_bind_int(stmt, 2, c.get_horaire_prevue());
    sqlite3_bind_text(stmt, 3, direction.c_str(), -1, SQLITE_TRANSIENT); // SQLITE_TRANSIENT car c'est une std::string temporaire
    sqlite3_bind_int(stmt, 4, id_lieu);
    sqlite3_bind_int(stmt, 5, c.get_taille());
    sqlite3_bind_int(stmt, 6, c.contient_urgence() ? 1 : 0);
    
    // NOUVEAU : Enregistrement de l'état (enum class castée en int)
    sqlite3_bind_int(stmt, 7, static_cast<int>(c.get_etat()));
    
    // NOUVEAU : Enregistrement de la région associée
    sqlite3_bind_int(stmt, 8, c.get_id_region());

    // 3. Exécution et libération de la mémoire
    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    
    if (!succes) {
        std::cerr << "[DAL] Erreur exécution ARCHIVE Convoi : " << sqlite3_errmsg(m_db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return succes;
}

int DalConvoi::compter_convois_journee(int jour_simulation) const {
    int total = 0;
    int debut_jour = jour_simulation * 1440; // 1440 minutes dans 24h
    int fin_jour = debut_jour + 1440;

    const char* sql = "SELECT COUNT(*) FROM dal_historique_convois WHERE horaire_depart_reel >= ? AND horaire_depart_reel < ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DAL] Erreur préparation COUNT Convois : " << sqlite3_errmsg(m_db) << std::endl;
        return total;
    }

    sqlite3_bind_int(stmt, 1, debut_jour);
    sqlite3_bind_int(stmt, 2, fin_jour);

    // SQLITE_ROW car le SELECT COUNT(*) renvoie exactement une ligne contenant le résultat
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0); // Index 0 car c'est la première (et unique) colonne demandée
    }

    sqlite3_finalize(stmt);
    return total;
}