#pragma once
#include <vector>
#include <unordered_map>
#include "../core/convoi.h"
#include "../core/Voiture.h"
#include "../core/Destination.h"
#include "../core/Cooperative.h"
#include "../core/PlageInterdite.h"

/**
 * @brief Planificateur central de la gare routière.
 * 
 * Responsable de la création des convois de départ et de retour,
 * du placement des créneaux sur le portail unique,
 * et de l'optimisation du planning selon la fonction objectif definie par 
 * << score = alpha(nb de passager) - beta(nb de convoie) - lambda(temps moyen d'attente des clients) >>
 */
class Planificateur {
public:
    /**
     * @brief Construit le planificateur avec les données de configuration.
     * @param destinations Map (id -> Destination) chargée depuis la configuration.
     * @param cooperatives Map (id -> Cooperative) chargée.
     * @param plages Liste des plages horaires interdites (gare principale).
     * @param parametres Map de paramètres (espacement, franchissement, seuils...).
     */
    Planificateur(const std::unordered_map<int, Destination>& destinations,
                  const std::unordered_map<int, Cooperative>& cooperatives,
                  const std::vector<PlageInterdite>& plages,
                  const std::unordered_map<std::string, int>& parametres);

    /**
     * @brief Lance la planification complète.
     * @param demande Nombre de passagers en attente par id_destination.
     * @param voitures_disponibles Liste des voitures actuellement à l'état EN_ATTENTE_GARE.
     * @param temps_courant Minute actuelle de la simulation (0..1439).
     * @return true si la planification a pu être réalisée, false sinon.
     */
    bool planifier(const std::unordered_map<int, int>& demande,
                   std::vector<Voiture*>& voitures_disponibles,
                   int temps_courant);

    /// @return Les convois de sortie planifiés (prêts à partir).
    const std::vector<Convoi>& get_convois_sortie() const { return m_convois_sortie; }
    /// @return Les convois d'entrée planifiés (retours prévus).
    const std::vector<Convoi>& get_convois_entree() const { return m_convois_entree; }

private:
    // Références vers les données de configuration (non modifiables)
    const std::unordered_map<int, Destination>& m_destinations;
    const std::unordered_map<int, Cooperative>& m_cooperatives;
    const std::vector<PlageInterdite>& m_plages;
    const std::unordered_map<std::string, int>& m_parametres;

    // Convois planifiés
    std::vector<Convoi> m_convois_sortie;
    std::vector<Convoi> m_convois_entree;

    // Agenda du portail : minute par minute, "true = occupé"
    std::vector<bool> m_agenda_portail;

    // Compteur d'ID pour les nouveaux convois
    int m_prochain_id_convoi = 1;

    // ─── Constantes issues des paramètres (cachées en privé) ───
    int m_espacement_min;            // minutes entre fin d'un convoi et début du suivant
    int m_franchissement_par_voiture; // minutes nécessaires par voiture pour le portail
    int m_delai_achat_min;           // un client doit acheter son billet au moins X min avant le départ
    int m_debut_journee;             // première minute de service (ex: 180 pour 3h00)
    int m_fin_journee;               // dernière minute de service (ex: 1380 pour 23h00)
    int m_taille_max_convoi;         // 8
    double m_seuil_remplissage_min;   // taux (ex: 0.2) pour accepter une voiture dans un convoi
    double m_seuil_critique;          // taux en dessous duquel un convoi peut être supprimé
    double m_poids_alpha;            // poids pour passagers transportés
    double m_poids_beta;             // poids pour pénalité par convoi
    double m_poids_gamma;            // poids pour retard moyen

    // ─── Méthodes privées de manipulation de l'agenda ───
    bool creneau_libre(int debut, int duree) const;
    void reserver_creneau(int debut, int duree);
    void liberer_creneau(int debut, int duree);

    // Vérifie si un intervalle chevauche une plage interdite
    bool chevauche_plage_interdite(int debut, int fin) const;

    // Recherche linéaire du premier créneau libre >= t_min
    int trouver_creneau(int t_min, int duree) const;

    // Phase 1 : construction gloutonne des convois de sortie
    std::vector<Convoi> former_convois_sortie(int id_dest, int& nb_passagers_restants,
                                              std::vector<Voiture*>& voitures_dispos);

    // Planifie le retour d'un convoi sortant (crée un convoi entrant)
    Convoi planifier_retour(const Convoi& convoi_sortie);

    // Phase 2 : optimisation locale
    void ameliorer_plan(std::vector<Convoi>& convois_sortie,
                        std::vector<Convoi>& convois_entree,
                        int temps_courant);

    // Calcul du score d'un ensemble de convois de sortie
    double calculer_score(const std::vector<Convoi>& convois_sortie, int temps_courant) const;
};