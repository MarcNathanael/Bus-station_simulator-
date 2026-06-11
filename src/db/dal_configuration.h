#pragma once
#include <string>
#include <unordered_map>
#include <sqlite3.h>

class DalConfiguration {
private:
    sqlite3* m_db;

public:
    explicit DalConfiguration(sqlite3* db);

    // Chargement de l'intégralité des paramètres en RAM au démarrage
    std::unordered_map<std::string, int> charger_parametres() const;

    // Insertion ou mise à jour d'un paramètre (UPSERT)
    // Utile pour le premier lancement depuis le CSV ou si tu modifies un paramètre via l'UI
    bool sauvegarder_parametre(const std::string& cle, int valeur);
};