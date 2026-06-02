#include "Billetterie.h"

/*constructeur vide cas std::vector<GroupeClients> m_carnet_reservations
le seule attribut est un vector qui se construit automatiquement 
*/
Billetterie::Billetterie() {}

void Billetterie::ajouter_reservations(const std::vector<GroupeClients>& nouveaux_clients) {
    for (size_t i = 0; i < nouveaux_clients.size(); ++i) {
        m_carnet_reservations.push_back(nouveaux_clients[i]);
    }
}

void Billetterie::extraire_demandes(double temps_courant, 
                                    std::unordered_map<int, int>& demande_depart_std, 
                                    std::unordered_map<int, int>& demande_depart_urg,
                                    std::unordered_map<int, int>& demande_retour_std,
                                    std::unordered_map<int, int>& demande_retour_urg) 
{
    // 1. Nettoyage des quais
    demande_depart_std.clear(); 
    demande_depart_urg.clear();
    demande_retour_std.clear(); 
    demande_retour_urg.clear();

    std::vector<GroupeClients> carnet_mis_a_jour;

    // 2. Tri des clients
    for (size_t i = 0; i < m_carnet_reservations.size(); ++i) {
        const GroupeClients& groupe = m_carnet_reservations[i];

        // RÈGLE : Si on est à 15 min (ou moins) de la fin de sa patience -> URGENT
        bool est_urgent = (temps_courant >= groupe.t_max - 15);
        // RÈGLE : Si l'heure idéale approche -> STANDARD
        bool est_standard = (temps_courant >= groupe.t_min - 30);

        if (est_urgent) {
            if (groupe.est_un_retour) demande_retour_urg[groupe.id_destination] += groupe.nb_passagers;
            else demande_depart_urg[groupe.id_destination] += groupe.nb_passagers;
        } 
        else if (est_standard) {
            if (groupe.est_un_retour) demande_retour_std[groupe.id_destination] += groupe.nb_passagers;
            else demande_depart_std[groupe.id_destination] += groupe.nb_passagers;
        } 
        else {
            // Pas encore l'heure, le client reste au chaud dans le carnet
            carnet_mis_a_jour.push_back(groupe);
        }
    }
    m_carnet_reservations = carnet_mis_a_jour; // ainsi la passager Tot seront a la tete de ceux qui viendront apres 
}

void Billetterie::traiter_demande_residuelle(double temps_courant,
                                             const std::unordered_map<int, int>& residus_depart,
                                             const std::unordered_map<int, int>& residus_retour) 
{
    // Les passagers rejetés reviennent ici.
    // On leur donne un nouveau t_max très court (ex: 15 min) pour forcer 
    // leur passage en URGENT au prochain tour 
    // .secon : nb_passager et .first : destination 
    for (auto paire : residus_depart) {
        if (paire.second > 0) 
        {
            m_carnet_reservations.push_back({paire.first, paire.second, static_cast<int>(temps_courant), static_cast<int>(temps_courant + 15), false});
        }
    }
    for (auto paire : residus_retour) {
        if (paire.second > 0) {
            m_carnet_reservations.push_back({paire.first, paire.second, static_cast<int>(temps_courant), static_cast<int>(temps_courant + 15), true});
        }
    }
}

// systeme entier que ca soit depart ou arriver 
int Billetterie::obtenir_charge_actuelle() const {
    int total = 0;
    for (size_t i = 0; i < m_carnet_reservations.size(); ++i) {
        total += m_carnet_reservations[i].nb_passagers;
    }
    return total;
}