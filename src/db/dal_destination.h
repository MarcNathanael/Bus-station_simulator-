#pragma once
#include <vector>
#include <sqlite3.h>
#include "../core/Destination.h"

class DalDestination {
private:
    sqlite3* m_db;

public:
    explicit DalDestination(sqlite3* db);

    // Chargement du référentiel en RAM au démarrage
    std::vector<Destination> charger_tout() const;

    // Insertion initiale (Utile lors de l'import CSV au premier lancement)
    bool inserer_destination(const Destination& d);
};