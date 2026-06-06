#ifndef SIMULATEUR_H
#define SIMULATEUR_H

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <iostream>

// Inclusions des classes métiers (basées sur ton architecture)
#include "Voiture.h"
#include "Convoi.h"
#include "PlageInterdite.h"
#include "Billetterie.h"
#include "Generateur.h"
#include "Planificateur.h"

class Simulateur {
private:
    // --- Horloge et Verrous Physiques ---
    int m_temps_courant;                        // T : Horloge continue absolue en minutes
    int m_portail_occupe_jusqua;                // Verrou cumulatif du portail de la gare principale
    int m_duree_franchissement_par_voiture;     // Durée de blocage du portail par véhicule (ex: 2 min)
    int m_frequence_planif;                     // Fréquence d'activation du Planificateur (ex: 30 min)

    // --- Collections de l'Environnement ---
    std::vector<Voiture*>& m_voitures_flotte;   // Référence vers la flotte globale
    std::vector<PlageInterdite> m_plages_interdites; // Plages de fermeture de la gare principale
    std::unordered_map<int, int> m_durees_trajet;    // Map associant [id_province] -> [duree_trajet en minutes]

    // --- Références aux Moteurs Métiers ---
    Billetterie& m_billetterie;
    GenerateurDemandes& m_generateur;
    Planificateur& m_planificateur;

    // --- Méthodes Physiques Internes ---
    /**
     * @brief Vérifie si la gare principale est fermée à l'instant T.
     */
    bool en_plage_interdite(int temps) const noexcept;

    /**
     * @brief Source de vérité : Synchronise l'état d'un véhicule vers la DB.
     */
    void mettre_a_jour_sqlite(const Voiture& voiture) const;

public:
    /**
     * @brief Constructeur du Simulateur.
     */
    Simulateur(std::vector<Voiture*>& flotte_globale,
               const std::vector<PlageInterdite>& plages,
               const std::unordered_map<int, int>& durees_trajet,
               Billetterie& billetterie,
               GenerateurDemandes& generateur,
               Planificateur& planificateur,
               int frequence_planif = 30,
               int duree_franchissement = 2);

    /**
     * @brief Exécute une étape temporelle unique (1 minute).
     * @param T Minute absolue actuelle.
     */
    void tick(int T);

    /**
     * @brief Lance la boucle de simulation sur une durée donnée.
     * @param duree_simulation Nombre total de minutes à simuler.
     */
    void executer(int duree_simulation);

    // --- Getters d'Observation ---
    int get_temps_courant() const noexcept { return m_temps_courant; }
    int get_portail_occupe_jusqua() const noexcept { return m_portail_occupe_jusqua; }
};

#endif // SIMULATEUR_H