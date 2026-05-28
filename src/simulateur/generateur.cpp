#include "generateur.h"
#include <algorithm>

GenerateurDemandes::GenerateurDemandes(int nb_total_voitures, int places_par_voiture, int graine)
    : m_generateur(graine) //la graine sert d'impulsion de depart , si on fixe la graine le resultat est reproductible 
{
    // Plafond = La gare peut accueillir au maximum 40% de la capacité totale de la flotte
    // invariant :
    m_plafond_gare = static_cast<int>(nb_total_voitures * places_par_voiture * 0.40);
}

void GenerateurDemandes::ajouter_destination(int id_dest, double lambda_base) {
    m_lambdas_base[id_dest] = lambda_base;//lambda popularitee pour chaque ville 
}

double GenerateurDemandes::obtenir_facteur_horaire(int temps_courant, bool est_un_retour) const 
{
    // En faisant % 1440, 1500 minutes deviennent 60 minutes (1h00 du matin, Jour 2)
    double heure = (temps_courant % 1440) / 60.0;    
    // Logique simplifiée (Pointe matin/soir)
    if (heure >= 23.0 || heure < 5.0) return 0.05; // Nuit
    if (est_un_retour && heure >= 6.5 && heure <= 9.0) return 2.5; // Pointe matin retours
    if (!est_un_retour && heure >= 16.0 && heure <= 19.0) return 3.0; // Pointe soir départs
    return 1.0;
}

void GenerateurDemandes::enregistrer_arrivee_province(int id_province, int nb_passagers, int temps_courant) {
    // Les voyageurs restent entre 4h (240 min) et 48h (2880 min) en province
    
    // c'est un foncteur avec une surchage d'operator() qui sert a calquer la valeur donner par m_generateur
    std::uniform_int_distribution<int> distrib_sejour(240, 2880);
    int fin_sejour = temps_courant + distrib_sejour(m_generateur);
    
    m_incubateur_province.push_back({id_province, nb_passagers, fin_sejour});
}

void GenerateurDemandes::generer_flux(int temps_courant, Billetterie& billetterie) 
{
    // 1. Vérification du Plafond d'Offre/Demande
    if (billetterie.obtenir_charge_actuelle() >= m_plafond_gare) 
    {
        return; // La gare est saturée, on ne génère pas de nouveaux clients
    }

    std::vector<GroupeClients> nouveaux_clients;

    // 2. Génération des Départs (Gare -> Province)
    for (auto paire : m_lambdas_base)
    {
        // .first :destination 
        // .second : populariter 
        int id_dest = paire.first;
        // lambda = populariter * lamda_horaire  
        double lambda_reel = paire.second * obtenir_facteur_horaire(temps_courant, false);// n'est pas un retour 

        if (lambda_reel > 0.0) 
        {
            // meme principe que celui de ::enregistrer_arrivee_province , on configure a l'initialisation avec le lambda
            std::poisson_distribution<int> distribution(lambda_reel);
            // on jette les des , plus lambda reel est grand plus le nombre trouver seras grand , m_genereteur n'est qu'une pulsion
            int nb_passagers = distribution(m_generateur);

            if (nb_passagers > 0) 
            {
                // Le client veut partir dans [30min, 4h] par rapport à l'heure d'achat
                // static pour creer l'objet qu'un seule fois en memoire
                static std::uniform_int_distribution<int> distrib_delai(30, 240);
                int t_min = temps_courant + distrib_delai(m_generateur);
                int t_max = t_min + 60; // 1 heures de patience max
                
                nouveaux_clients.push_back({id_dest, nb_passagers, t_min, t_max, false});
            }
        }
    }

    // 3. Génération des Retours (Ceux dont le séjour est terminé)
    std::vector<Sejour> sejours_restants;
    for (size_t i = 0; i < m_incubateur_province.size(); ++i) 
    {
        Sejour& s = m_incubateur_province[i];
        // le temps de fin de sejour peut aller jusqu'a 48h on peut pas directement comparer de cette maniere !!!
        if (temps_courant >= s.temps_fin_sejour) 
        {
            // Le séjour est fini, on crée la demande de retour immédiate
            // (La station de province est petite, on suppose qu'ils veulent partir vite)
            int t_min = temps_courant + 15;
            int t_max = temps_courant + 60; // Patience de 1h en province
            nouveaux_clients.push_back({s.id_province, s.nb_passagers, t_min, t_max, true});
        } else 
        {
            sejours_restants.push_back(s);
        }
    }
    m_incubateur_province = sejours_restants;

    // 4. Envoi à la billetterie
    if (!nouveaux_clients.empty()) 
    {
        billetterie.ajouter_reservations(nouveaux_clients);
    }
}