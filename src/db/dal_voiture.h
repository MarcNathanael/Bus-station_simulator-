#pragma once
#include <vector>
#include <sqlite3.h>
#include "../core/Voiture.h" // Assure-toi du bon chemin vers tes modèles métiers

class DalVoiture {
private:
    sqlite3* m_db; // Référence vers la connexion gérée par DatabaseManager

public:
    explicit DalVoiture(sqlite3* db);

    // ÉTAPE 3 : Chargement initial (SQLite -> RAM)
    std::vector<Voiture> charger_tout() const;

    // ÉTAPE 4 : Mise à jour différée (RAM -> SQLite)
    bool mettre_a_jour_voiture(const Voiture& v);
    
    // (Optionnel pour l'initialisation) Insertion massive
    bool inserer_voiture(const Voiture& v); 
};