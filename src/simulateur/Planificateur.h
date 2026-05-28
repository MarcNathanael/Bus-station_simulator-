#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <unordered_set>
#include <set> // Indispensable pour la fusion des maps
#include "../core/convoi.h"
#include "../core/Voiture.h"
#include "../core/Destination.h"
#include "../core/Cooperative.h"
#include "../core/PlageInterdite.h"

class Planificateur {
public:
    // Constructeur : initialise le planificateur avec les données de la gare
    Planificateur(const std::unordered_map<int, Destination>& destinations,
                  const std::unordered_map<int, Cooperative>& cooperatives,
                  const std::vector<PlageInterdite>& plages,
                  const std::unordered_map<std::string, int>& parametres);

    // Méthode principale qui lance la planification
    // Le planificateur global reçoit désormais 4 listes au lieu de 2
    bool planifier_global(
        const std::unordered_map<int, int>& demande_depart_std,
        const std::unordered_map<int, int>& demande_depart_urgente,
        const std::unordered_map<int, int>& demande_retour_std,
        const std::unordered_map<int, int>& demande_retour_urgente,
        std::vector<Voiture*>& voitures_gare,
        const std::unordered_map<int, std::vector<Voiture*>>& voitures_par_province,
        int temps_courant);

    std::pair<std::unordered_map<int, int>, std::unordered_map<int, int>>
    calculer_demande_residuelle(
        const std::unordered_map<int, int>& demande_depart_initiale,
        const std::unordered_map<int, int>& demande_retour_initiale,
        const std::vector<Convoi>& convois_sortie,
        const std::vector<Convoi>& convois_entree);

    // Fonctions pour récupérer les résultats
    const std::vector<Convoi>& get_convois_sortie() const { return m_convois_sortie; }
    const std::vector<Convoi>& get_convois_entree() const { return m_convois_entree; }

private:
    // Données de configuration reçues du système
    const std::unordered_map<int, Destination>& m_destinations;
    const std::unordered_map<int, Cooperative>& m_cooperatives;
    const std::vector<PlageInterdite>& m_plages;
    const std::unordered_map<std::string, int>& m_parametres;

    // Listes finales des convois planifiés
    std::vector<Convoi> m_convois_sortie;
    std::vector<Convoi> m_convois_entree;

    // sac infinie 
    std::unordered_set<int> m_agenda;
    int m_prochain_id_convoi = 1;

    // Variables de configuration (seuils, durées, poids du score)
    int m_espacement_min;
    int m_franchissement_par_voiture;
    int m_delai_achat_min;
    int m_debut_journee;
    int m_fin_journee;
    int m_taille_max_convoi;
    double m_seuil_remplissage_min;
    double m_seuil_critique;
    double m_poids_alpha, m_poids_beta, m_poids_gamma;

    // Outils pour gérer le temps et l'agenda
    bool creneau_libre(int debut, int duree) const;
    bool chevauche_plage_interdite(int debut, int fin) const;
    void reserver_creneau(int debut, int duree);
    void liberer_creneau(int debut, int duree);
    int trouver_creneau(int t_min, int duree) const;

    // Fonction d'aide pour lire les paramètres facilement
    int lire_parametre(const std::string& cle, int valeur_par_defaut);

    // Fonctions pour créer les convois en remplissant les voitures
   // On remplace 'passagers_restants' par deux compteurs distincts
std::vector<Convoi> former_convois_sortie(int id_dest,
                                          int& passagers_urgents,
                                          int& passagers_standards,
                                          std::vector<Voiture*>& voitures_disponibles,
                                          std::unordered_map<Voiture*, int>& historiques);

// Même chose pour le retour
std::vector<Convoi> former_convois_retour(int id_province,
                                          int& passagers_urgents,
                                          int& passagers_standards,
                                          std::vector<Voiture*>& voitures_disponibles,
                                          std::unordered_map<Voiture*, int>& historiques);

    // Gestion des conflits (décaler un convoi pour en placer un autre)
    bool reparer_et_inserer(Convoi& nouveau, std::vector<Convoi>& places, int temps_courant);

    // Optimisation du planning (Fusions, Décalages, Suppressions)
    void ameliorer_plan_global(int temps_courant);

    // Calcul de la performance du planning
    double calculer_score(const std::vector<Convoi>& sorties, const std::vector<Convoi>& entrees, int temps_courant) const;

    void Planificateur::nettoyer_convois_passes(int temps_courant);
};