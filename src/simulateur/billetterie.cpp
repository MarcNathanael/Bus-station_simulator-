#include "Billetterie.h"

Billetterie::Billetterie() {}

void Billetterie::ajouter_reservations(const std::vector<GroupeClients>& nouveaux_clients) {
    for (size_t i = 0; i < nouveaux_clients.size(); ++i) {
        m_carnet_reservations.push_back(nouveaux_clients[i]);
    }
}

void Billetterie::extraire_demandes_imminentes(int temps_courant, 
                                               std::unordered_map<int, int>& demande_depart, 
                                               std::unordered_map<int, int>& demande_retour) 
{
    // On vide les maps pour le Planificateur
    demande_depart.clear();
    demande_retour.clear();

    std::vector<GroupeClients> carnet_mis_a_jour;

    for (size_t i = 0; i < m_carnet_reservations.size(); ++i) {
        const GroupeClients& groupe = m_carnet_reservations[i];

        // On réveille le client 30 minutes avant son t_min
        if (temps_courant >= groupe.t_min - 30) {
            if (groupe.est_un_retour) {
                demande_retour[groupe.id_destination] += groupe.nb_passagers;
            } else {
                demande_depart[groupe.id_destination] += groupe.nb_passagers;
            }
        } else {
            // Il est trop tôt, il reste dans le carnet
            carnet_mis_a_jour.push_back(groupe);
        }
    }

    // On met à jour le carnet (ceux qui sont partis sur le quai n'y sont plus)
    m_carnet_reservations = carnet_mis_a_jour;
}

void Billetterie::traiter_demande_residuelle(int temps_courant,
                                             const std::unordered_map<int, int>& residus_depart,
                                             const std::unordered_map<int, int>& residus_retour) 
{
    // Fonction simple : On recrée des groupes pour les résidus avec une patience de 60 minutes max.
    // Si la billetterie avait gardé l'ID précis, on aurait checké leur vrai t_max, 
    // mais ici on simule "Je rate mon convoi, j'attends le prochain (max 1h)".
    
    for (auto paire : residus_depart) {
        int id_dest = paire.first;
        int nb_passagers = paire.second;
        if (nb_passagers > 0) {
            GroupeClients repousse = {id_dest, nb_passagers, temps_courant, temps_courant + 60, false};
            m_carnet_reservations.push_back(repousse);
        }
    }

    for (auto paire : residus_retour) {
        int id_prov = paire.first;
        int nb_passagers = paire.second;
        if (nb_passagers > 0) {
            GroupeClients repousse = {id_prov, nb_passagers, temps_courant, temps_courant + 60, true};
            m_carnet_reservations.push_back(repousse);
        }
    }
}

int Billetterie::obtenir_charge_actuelle() const {
    int total = 0;
    for (size_t i = 0; i < m_carnet_reservations.size(); ++i) {
        total += m_carnet_reservations[i].nb_passagers;
    }
    return total;
}