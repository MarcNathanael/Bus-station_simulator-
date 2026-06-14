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
#include "db/DatabaseManager.h"
#include "db/dal_voiture.h"
#include "db/dal_convoi.h"  
#include "db/dal_destination.h"
#include "db/dal_cooperative.h"
#include "db/dal_plage_interdite.h"
#include "db/dal_configuration.h"


class Simulateur 
{
    private:
        // --- Horloge et Verrous Physiques ---
        int m_temps_continue;                        // T : Horloge continue absolue en minutes
        int m_portail_occupe_jusqua;                // Verrou cumulatif du portail de la gare principale
        int m_duree_franchissement_par_voiture;     // Durée de blocage du portail par véhicule (ex: 2 min)
        int m_frequence_planif;                     // Fréquence d'activation du Planificateur (ex: 30 min)
        int m_origine;

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
         * @brief Vérifie si le portail de la  gare principale est fermée à l'instant T.
         */
        bool en_plage_interdite(int temps) const noexcept;

        /**
         * @brief Source de vérité : Synchronise l'état d'un véhicule vers la DB.
         */

        // data :
        DatabaseManager* m_dbManager;
        DalVoiture* m_dalVoiture; // Instanciée dans le constructeur avec la connexion
        DalConvoi* m_dalConvoi;

        // Fonction de synchronisation par lots
        void synchroniser_bdd();
        /**
         * @brief Vérifie la BDD, amorce via les CSV si nécessaire,
         * et extrait les données de SQLite vers la RAM (Cache de simulation).
         */
        bool Simulateur::orchestrer_demarrage(
            DatabaseManager& db, 
            std::vector<Voiture>& conteneur_physique, 
            std::vector<Voiture*>& flotte_pointeurs,
            std::vector<Destination>& destinations_ram,
            std::vector<Cooperative>& cooperatives_ram,
            std::vector<PlageInterdite>& plages_ram,
            std::unordered_map<std::string, int>& parametres_ram);
    public:
        /**
         * @brief Constructeur du Simulateur.
         */
        Simulateur(int id_origine,
                std::vector<Voiture*>& flotte_globale,
                const std::vector<PlageInterdite>& plages,
                const std::unordered_map<int, int>& durees_trajet,
                Billetterie& billetterie,
                GenerateurDemandes& generateur,
                Planificateur& planificateur,
                int frequence_planif = 30,// plus besoin de mettre dans le constructeur 
                int duree_franchissement = 2,
                DatabaseManager* m_dbManager,
                DalVoiture* m_dalVoiture,
                DalConvoi* dalConvoi
            );

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
        int get_temps_continue() const noexcept { return m_temps_continue; }
        int get_portail_occupe_jusqua() const noexcept { return m_portail_occupe_jusqua; }
};

#endif // SIMULATEUR_H