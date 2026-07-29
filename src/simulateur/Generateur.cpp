#include "Generateur.h"
#include <iostream>
#include <algorithm>
#include <cmath>
GenerateurDemandes::GenerateurDemandes(int nb_total_voitures, int places_par_voiture, int graine)
    : m_generateur(graine) //la graine sert d'impulsion de depart , si on fixe la graine le resultat est reproductible 
{
    // Plafond = La gare peut accueillir au maximum 40% de la capacité totale de la flotte
    m_plafond_gare = static_cast<int>(nb_total_voitures * places_par_voiture * CAPACITER_MAX);
}

void GenerateurDemandes::ajouter_destination(int id_dest, double lambda_base) {
    m_lambdas_base[id_dest] = lambda_base;//lambda popularitee pour chaque ville 
}

double GenerateurDemandes::obtenir_facteur_horaire(double temps_continu, bool est_un_retour) const 
{
    // fmod(x, 1440.0) remplace le x % 1440 pour les nombres à virgule.
    double minutes_dans_journee = std::fmod(temps_continu, 1440.0);
    
    // SÉCURITÉ C++ : Si temps_continu est négatif, fmod renvoie une valeur négative.
    // Ce mini-ajustement garantit qu'on reste sur une horloge cyclique positive de 0 à 1440.
    if (minutes_dans_journee < 0.0) {
        minutes_dans_journee += 1440.0;
    }

    double heure = minutes_dans_journee / 60.0;

    if (heure >= 23.0 || heure < 5.0) return POID_NUIT;                  // Nuit
    if (est_un_retour && heure >= 6.5 && heure <= 9.0) return POID_RETOUR;   // Pointe matin retours
    if (!est_un_retour && heure >= 16.0 && heure <= 19.0) return POID_SORTIE; // Pointe soir départs
    
    return 1.0; // par defaut
}

void GenerateurDemandes::enregistrer_arrivee_province(int id_province, int nb_passagers, double temps_continu) 
{
    // Les voyageurs restent entre 4h (240 min) et 48h (2880 min) en province
    
    // c'est un foncteur avec une surchage d'operator() qui sert a calquer la valeur donner par m_generateur
    std::uniform_int_distribution<int> distrib_sejour(SEJOUR_MIN, SEJOUR_MAX);
    int fin_sejour = temps_continu + distrib_sejour(m_generateur);
    m_incubateur_province.push_back({id_province, nb_passagers, fin_sejour});
}


void GenerateurDemandes::generer_flux(double temps_continu, 
                                      Billetterie& billetterie, 
                                      DalClient* dalClient, 
                                      int& prochain_id_client) 
{
    // 1. Vérification du Plafond d'Offre/Demande à la gare principale
    if (billetterie.obtenir_charge_actuelle(temps_continu) >= m_plafond_gare) 
    {
        std::cout << "[GENERATEUR] T=" << temps_continu << " PLAFOND ATTEINT (" << billetterie.obtenir_charge_actuelle(temps_continu) << "/" << m_plafond_gare << "). Generation bloquee." << std::endl;
        return; 
    }

    std::vector<GroupeClients> nouveaux_clients;

    // ====================================================================
    // 2. Génération des Départs Autonomes (Gare Principale -> Province)
    // ====================================================================
    for (auto paire : m_lambdas_base)
    {
        int id_dest = paire.first;
        double lambda_reel_depart = paire.second * obtenir_facteur_horaire(temps_continu, false) * m_multiplicateur_flux;

        if (lambda_reel_depart > 0.0) 
        {
            std::poisson_distribution<int> distribution(lambda_reel_depart);
            int nb_passagers = distribution(m_generateur);

            if (nb_passagers > 0) 
            {
                static std::uniform_int_distribution<int> distrib_delai(DELAI_MIN, DELAI_MAX);
                int t_min = temps_continu + distrib_delai(m_generateur);
                int t_max = t_min + MAX_PATIENCE; 
                
                nouveaux_clients.push_back({id_dest, nb_passagers, t_min, t_max, false});
                std::cout << "[GENERATEUR] T=" << temps_continu << " Departs auto: " << nb_passagers << " passagers vers Dest=" << id_dest << std::endl;
            }
        }
    }

    // ====================================================================
    // 3. Génération des Retours Autonomes (Province -> Gare Principale)
    // ====================================================================
    for (auto paire : m_lambdas_base)
    {
        int id_province = paire.first;
        double lambda_reel_retour = paire.second * obtenir_facteur_horaire(temps_continu, true) * m_multiplicateur_flux;

        if (lambda_reel_retour > 0.0) 
        {
            std::poisson_distribution<int> distribution(lambda_reel_retour);
            int nb_passagers = distribution(m_generateur);

            if (nb_passagers > 0) 
            {
                static std::uniform_int_distribution<int> distrib_delai_retour(DELAI_MIN, DELAI_MAX);
                int t_min = temps_continu + distrib_delai_retour(m_generateur);
                int t_max = t_min + MAX_PATIENCE; 
                
                nouveaux_clients.push_back({id_province, nb_passagers, t_min, t_max, true});
                std::cout << "[GENERATEUR] T=" << temps_continu << " Retours auto: " << nb_passagers << " passagers depuis Prov=" << id_province << std::endl;
            }
        }
    }

    // ====================================================================
    // 4. Ajout des Retours "Incubés" (Fin de séjour)
    // ====================================================================
    std::vector<Sejour> sejours_restants;
    for (size_t i = 0; i < m_incubateur_province.size(); ++i) 
    {
        Sejour& s = m_incubateur_province[i];
        
        if (temps_continu >= s.temps_fin_sejour) 
        {
            int t_min = temps_continu + MIN_PATIENCE;
            int t_max = temps_continu + MAX_PATIENCE; 
            nouveaux_clients.push_back({s.id_province, s.nb_passagers, t_min, t_max, true});
            std::cout << "[GENERATEUR] T=" << temps_continu << " Fin de sejour: " << s.nb_passagers << " passagers retournent de Prov=" << s.id_province << std::endl;
        } else 
        {
            sejours_restants.push_back(s);
        }
    }
    m_incubateur_province = sejours_restants;

    // ====================================================================
    // 5. Envoi à la billetterie (RELAIS DE LA DAL ET DE L'ID CONSERVÉS)
    // ====================================================================
    if (!nouveaux_clients.empty()) 
    {
        std::cout << "[GENERATEUR] T=" << temps_continu << " Envoi de " << nouveaux_clients.size() << " groupes a la billetterie." << std::endl;
        billetterie.ajouter_reservations(nouveaux_clients, dalClient, prochain_id_client);
    }
}