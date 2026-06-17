#pragma once
#include <vector>
#include <sqlite3.h>
#include "../core/Convoi.h"

class DalConvoi {
private:
    sqlite3* m_db;

public:
    explicit DalConvoi(sqlite3* db);

    int get_max_id_convoi() const;
    
    // Fonction appelée par le Write-Behind lorsqu'un convoi passe en EtatConvoi::TERMINE
    // On peut y ajouter un paramètre 'score' si on le calcule à ce moment-là
    bool archiver_convoi(const Convoi& c );
    
    // Pour la future UI : récupérer le nombre de convois sur une période
    int compter_convois_journee(int jour_simulation) const;
};