#pragma once
#include "structures.h"
#include <vector>
#include <unordered_map>

class Billetterie {
public:
    Billetterie();

    // Reçoit les nouveaux clients du générateur et les stocke
    void ajouter_reservations(const std::vector<GroupeClients>& nouveaux_clients);

    // Prépare les map pour le Planificateur en extrayant les clients dont l'heure approche
    void extraire_demandes_imminentes(int temps_courant, 
                                      std::unordered_map<int, int>& demande_depart, 
                                      std::unordered_map<int, int>& demande_retour);

    // Gère les clients rejetés par le Planificateur
    void traiter_demande_residuelle(int temps_courant,
                                    const std::unordered_map<int, int>& residus_depart,
                                    const std::unordered_map<int, int>& residus_retour);

    // Retourne le nombre total de personnes actuellement dans le système
    int obtenir_charge_actuelle() const;

private:
    std::vector<GroupeClients> m_carnet_reservations;
};