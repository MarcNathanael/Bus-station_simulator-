#include "dal_voiture.h"
#include <iostream>

// Mise à jour du constructeur pour intercepter et stocker les configurations
DalVoiture::DalVoiture(sqlite3* db, int t_chargement, int t_dechargement) 
    : m_db(db), 
      m_temps_chargement(t_chargement), 
      m_temps_dechargement(t_dechargement) 
{
}

std::vector<Voiture> DalVoiture::charger_tout() const {
    std::vector<Voiture> flotte;
    
    // 1. La requête SQL sélectionne TOUS les attributs nécessaires
    const char* sql = "SELECT id, id_coop, id_destination, id_position, "
                      "capacite_max, places_libres, etat, horaire_depart, heure_arrivee "
                      "FROM dal_voitures;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Erreur préparation SELECT : " << sqlite3_errmsg(m_db) << std::endl;
        return flotte;
    }

    // 2. Lecture ligne par ligne
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id            = sqlite3_column_int(stmt, 0);
        int id_coop       = sqlite3_column_int(stmt, 1);
        int id_dest       = sqlite3_column_int(stmt, 2);
        int id_pos        = sqlite3_column_int(stmt, 3);
        int capacite_max  = sqlite3_column_int(stmt, 4);
        int places_libres = sqlite3_column_int(stmt, 5);
        
        EtatVoiture etat  = static_cast<EtatVoiture>(sqlite3_column_int(stmt, 6));
        
        int horaire_dep   = sqlite3_column_int(stmt, 7);
        double heure_arr  = sqlite3_column_double(stmt, 8); 

        // 3. Utilisation du nouveau constructeur injecté avec m_temps_chargement et m_temps_dechargement
        Voiture v(id, id_coop, id_dest, id_pos, capacite_max, places_libres, etat, horaire_dep, 
                  m_temps_chargement, m_temps_dechargement);
        
        v.set_heure_arrivee(heure_arr);
        flotte.push_back(v);
    }

    sqlite3_finalize(stmt);
    return flotte;
}

bool DalVoiture::mettre_a_jour_voiture(const Voiture& v) {
    const char* sql = "UPDATE dal_voitures SET "
                      "id_coop = ?, "
                      "places_libres = ?, "
                      "etat = ?, "
                      "id_position = ?, "
                      "id_destination = ?, "
                      "horaire_depart = ?, "
                      "heure_arrivee = ? "
                      "WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Erreur préparation UPDATE : " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, v.get_id_coop());
    sqlite3_bind_int(stmt, 2, v.get_places_libres());
    sqlite3_bind_int(stmt, 3, static_cast<int>(v.get_etat()));
    sqlite3_bind_int(stmt, 4, v.get_position());
    sqlite3_bind_int(stmt, 5, v.get_destination());
    sqlite3_bind_int(stmt, 6, v.get_horaire_depart());
    sqlite3_bind_double(stmt, 7, v.get_heure_arrivee()); 
    
    sqlite3_bind_int(stmt, 8, v.get_id());

    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    
    if (!succes) {
        std::cerr << "Erreur exécution UPDATE : " << sqlite3_errmsg(m_db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return succes;
}