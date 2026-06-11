#pragma once
#include <vector>
#include <sqlite3.h>
#include "../core/PlageInterdite.h"

class DalPlageInterdite {
private:
    sqlite3* m_db;

public:
    explicit DalPlageInterdite(sqlite3* db);

    // Chargement en RAM au démarrage du Simulateur
    std::vector<PlageInterdite> charger_tout() const;

    // Utile pour l'initialisation depuis le CSV "plages_interdites.csv"
    bool inserer_plage(const PlageInterdite& plage);
};