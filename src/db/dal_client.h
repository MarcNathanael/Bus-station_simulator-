#pragma once
#include <vector>
#include <sqlite3.h>
#include "../core/Client.h"

class DalClient {
private:
    sqlite3* m_db;

public:
    explicit DalClient(sqlite3* db);

    // Permet de recharger la file d'attente en cas de crash
    std::vector<Client> charger_tout() const;

    // Flush Write-Behind : Ajoute un nouveau client généré par le Générateur
    bool inserer_client(const Client& c);

    // Flush Write-Behind : Retire le client de la BDD quand il monte dans un convoi
    bool supprimer_client(int id_client);
};