#pragma once
#include <vector>
#include <sqlite3.h>
#include "../core/Billet.h"

class DalBillet {
private:
    sqlite3* m_db;

public:
    explicit DalBillet(sqlite3* db);

    // Fonction principale appelée lors du Flush Write-Behind
    bool inserer_billet(const Billet& b);
    
    // Utile pour la future interface graphique (Statistiques)
    int compter_billets_vendus_journee(int jour_simulation) const;
};