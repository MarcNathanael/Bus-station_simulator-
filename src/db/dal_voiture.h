#ifndef DAL_VOITURE_H
#define DAL_VOITURE_H

#include <vector>
#include <sqlite3.h>
#include "../core/Voiture.h"

class DalVoiture {
private:
    sqlite3* m_db;
    
    // AJOUT : Stockage des configurations globales de temps pour la reconstruction
    int m_temps_chargement;
    int m_temps_dechargement;

public:
    // Le constructeur reçoit maintenant la connexion ET les paramètres de temps
    DalVoiture(sqlite3* db, int t_chargement, int t_dechargement);

    std::vector<Voiture> charger_tout() const;
    bool mettre_a_jour_voiture(const Voiture& v);
};

#endif // DAL_VOITURE_H