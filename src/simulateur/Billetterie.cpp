#include "Billetterie.h"

/*constructeur vide cas std::vector<GroupeClients> m_carnet_reservations
le seule attribut est un vector qui se construit automatiquement 
*/
Billetterie::Billetterie() {}

void Billetterie::ajouter_reservations(const std::vector<GroupeClients>& nouveaux_clients, 
                                      DalClient* dalClient, 
                                      int& prochain_id_client) 
{
    for (size_t i = 0; i < nouveaux_clients.size(); ++i) {
        const GroupeClients& groupe = nouveaux_clients[i];
        
        // 1. On garde la logique interne macro en RAM pour le simulateur
        m_carnet_reservations.push_back(groupe);

        // 2. LIAISON DAL : Si la base est active, on convertit le groupe anonyme en clients réels
        if (dalClient) {
            for (int j = 0; j < groupe.nb_passagers; ++j) {
                // On éclate le groupe en 'N' passagers individuels avec un ID unique
                Client c(prochain_id_client++, groupe.id_destination, groupe.t_min, groupe.t_max, groupe.est_un_retour);
                
                // Sauvegarde immédiate dans la table 'dal_clients_attente'
                dalClient->inserer_client(c);
            }
        }
    }
}

void Billetterie::extraire_demandes(double temps_continu, 
                                    std::unordered_map<int, int>& demande_depart_std, 
                                    std::unordered_map<int, int>& demande_depart_urg,
                                    std::unordered_map<int, int>& demande_retour_std,
                                    std::unordered_map<int, int>& demande_retour_urg) 
{
    demande_depart_std.clear(); 
    demande_depart_urg.clear();
    demande_retour_std.clear(); 
    demande_retour_urg.clear();

    std::vector<GroupeClients> carnet_mis_a_jour;
    int total_std = 0, total_urg = 0;

    for (size_t i = 0; i < m_carnet_reservations.size(); ++i) {
        const GroupeClients& groupe = m_carnet_reservations[i];

        bool est_urgent = (temps_continu >= groupe.t_max - 15);
        bool est_standard = (temps_continu >= groupe.t_min - 30);

        if (est_urgent) {
            if (groupe.est_un_retour) demande_retour_urg[groupe.id_destination] += groupe.nb_passagers;
            else demande_depart_urg[groupe.id_destination] += groupe.nb_passagers;
            total_urg += groupe.nb_passagers;
        } 
        else if (est_standard) {
            if (groupe.est_un_retour) demande_retour_std[groupe.id_destination] += groupe.nb_passagers;
            else demande_depart_std[groupe.id_destination] += groupe.nb_passagers;
            total_std += groupe.nb_passagers;
        } 
        else {
            carnet_mis_a_jour.push_back(groupe);
        }
    }
    m_carnet_reservations = carnet_mis_a_jour; 
    
    if (total_std > 0 || total_urg > 0) {
        std::cout << "[BILLETTERIE] T=" << temps_continu << " Extraction: " << total_std << " std, " << total_urg << " urg" << std::endl;
    }
}

void Billetterie::traiter_demande_residuelle(double temps_continu,
                                             const std::unordered_map<int, int>& residus_depart,
                                             const std::unordered_map<int, int>& residus_retour) 
{
    for (auto paire : residus_depart) {
        if (paire.second > 0 && paire.first != 0) 
        {
            m_carnet_reservations.push_back({paire.first, paire.second, static_cast<int>(temps_continu), static_cast<int>(temps_continu + 50), false});
        }
    }
    for (auto paire : residus_retour) {
        if (paire.second > 0 && paire.first != 0) {
            m_carnet_reservations.push_back({paire.first, paire.second, static_cast<int>(temps_continu), static_cast<int>(temps_continu + 50), true});
        }
    }
}

// systeme entier que ca soit depart ou arriver 
// systeme entier que ca soit depart ou arriver 
int Billetterie::obtenir_charge_actuelle(double temps_continu) const {
    int total = 0;
    for (size_t i = 0; i < m_carnet_reservations.size(); ++i) {
        // CORRECTION CRITIQUE : Le plafond de la gare ne concerne QUE les passagers attendant un départ (Gare -> Province).
        // Les passagers attendant un retour (Province -> Gare) sont physiquement en province et n'occupent pas la gare.
        if (!m_carnet_reservations[i].est_un_retour) {
            if (temps_continu >= m_carnet_reservations[i].t_min - 30.0) {
                total += m_carnet_reservations[i].nb_passagers;
            }
        }
    }
    return total;
}
void Billetterie::obtenir_compteurs_attente(double temps_continu, int& total_std, int& total_urg) const {
    total_std = 0;
    total_urg = 0;

    for (size_t i = 0; i < m_carnet_reservations.size(); ++i) {
        const GroupeClients& groupe = m_carnet_reservations[i];
        
        // On réplique la logique métier exacte de extraire_demandes()
        bool est_urgent = (temps_continu >= groupe.t_max - 15);
        bool est_standard = (temps_continu >= groupe.t_min - 30);

        if (est_urgent) {
            total_urg += groupe.nb_passagers;
        } else if (est_standard) {
            total_std += groupe.nb_passagers;
        }
        // Si non standard et non urgent, le passager est "futur" et n'est pas compté dans la file active
    }
}

std::unordered_map<int, std::pair<int, int>> Billetterie::obtenir_demandes_par_dest(double temps_continu) const {
    std::unordered_map<int, std::pair<int, int>> demandes;

    for (size_t i = 0; i < m_carnet_reservations.size(); ++i) {
        const GroupeClients& groupe = m_carnet_reservations[i];
        
        bool est_urgent = (temps_continu >= groupe.t_max - 15);
        bool est_standard = (temps_continu >= groupe.t_min - 30);

        if (est_urgent) {
            demandes[groupe.id_destination].second += groupe.nb_passagers; // .second = Urgents
        } else if (est_standard) {
            demandes[groupe.id_destination].first += groupe.nb_passagers;  // .first = Standards
        }
    }
    
    return demandes;
}