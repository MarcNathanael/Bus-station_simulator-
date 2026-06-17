#pragma once
#include "Structures.h"
#include "../core/Client.h"
#include "../db/dal_client.h"
#include <vector>
#include <unordered_map>

class Billetterie {
public:
    Billetterie();
    void ajouter_reservations(const std::vector<GroupeClients>& nouveaux_clients, DalClient* dalClient, 
                        int& prochain_id_client);
    
    // NOUVELLE SIGNATURE : Sépare les flux Standards et Urgents
    void extraire_demandes(double temps_continu, 
                           std::unordered_map<int, int>& demande_depart_std, 
                           std::unordered_map<int, int>& demande_depart_urg,
                           std::unordered_map<int, int>& demande_retour_std,
                           std::unordered_map<int, int>& demande_retour_urg);

    void traiter_demande_residuelle(double temps_continu,
                                    const std::unordered_map<int, int>& residus_depart,
                                    const std::unordered_map<int, int>& residus_retour);
    int obtenir_charge_actuelle() const;

private:
    std::vector<GroupeClients> m_carnet_reservations;
};