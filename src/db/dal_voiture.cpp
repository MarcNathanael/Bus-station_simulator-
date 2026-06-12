#include "dal_voiture.h"
#include <iostream>

DalVoiture::DalVoiture(sqlite3* db) : m_db(db) {}

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
        // Extraction par index (0 à 8)
        int id            = sqlite3_column_int(stmt, 0);
        int id_coop       = sqlite3_column_int(stmt, 1);
        int id_dest       = sqlite3_column_int(stmt, 2);
        int id_pos        = sqlite3_column_int(stmt, 3);
        int capacite_max  = sqlite3_column_int(stmt, 4);
        int places_libres = sqlite3_column_int(stmt, 5);
        
        // On récupère l'entier pour le transformer en enum class EtatVoiture
        EtatVoiture etat  = static_cast<EtatVoiture>(sqlite3_column_int(stmt, 6));
        
        int horaire_dep   = sqlite3_column_int(stmt, 7);
        double heure_arr  = sqlite3_column_double(stmt, 8); // Attention : sqlite3_column_double pour le type double !

        // 3. Utilisation du constructeur complet de ta classe Voiture
        Voiture v(id, id_coop, id_dest, id_pos, capacite_max, places_libres, etat, horaire_dep);
        
        // Comme heure_arrivee n'est pas dans le constructeur complet, on passe par son setter
        v.set_heure_arrivee(heure_arr);

        flotte.push_back(v);
    }

    sqlite3_finalize(stmt);
    return flotte;
}

bool DalVoiture::mettre_a_jour_voiture(const Voiture& v) {
    // 1. Le modèle SQL met à jour tout ce qui peut changer en cours de route
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

    // 2. Association rigoureuse des valeurs aux trous '?' (De 1 à 8)
    sqlite3_bind_int(stmt, 1, v.get_id_coop());
    sqlite3_bind_int(stmt, 2, v.get_places_libres());
    sqlite3_bind_int(stmt, 3, static_cast<int>(v.get_etat()));
    sqlite3_bind_int(stmt, 4, v.get_position());
    sqlite3_bind_int(stmt, 5, v.get_destination());
    sqlite3_bind_int(stmt, 6, v.get_horaire_depart());
    sqlite3_bind_double(stmt, 7, v.get_heure_arrivee()); // sqlite3_bind_double pour insérer un double
    
    // Le WHERE id = ? prend le dernier marqueur
    sqlite3_bind_int(stmt, 8, v.get_id());

    // 3. Exécution
    bool succes = (sqlite3_step(stmt) == SQLITE_DONE);
    
    if (!succes) {
        std::cerr << "Erreur exécution UPDATE : " << sqlite3_errmsg(m_db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return succes;
}