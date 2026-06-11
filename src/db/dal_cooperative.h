#pragma once
#include <vector>
#include <sqlite3.h>
#include "../core/Cooperative.h"

class DalCooperative {
private:
    sqlite3* m_db;

public:
    explicit DalCooperative(sqlite3* db);

    std::vector<Cooperative> charger_tout() const;
    bool inserer_cooperative(const Cooperative& c);
};