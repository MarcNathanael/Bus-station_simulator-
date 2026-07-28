#ifndef SIMULATEUR_H
#define SIMULATEUR_H

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <iostream>

// Inclusions des classes métiers (basées sur ton architecture)
#include "../core/Voiture.h"
#include "../core/Convoi.h"
#include "../core/PlageInterdite.h"
#include "../simulateur/Billetterie.h"
#include "../simulateur/Generateur.h"
#include "../simulateur/Planificateur.h"
#include "../core/Billet.h"
#include "../db/DatabaseManager.h"
#include "../db/dal_voiture.h"
#include "../db/dal_convoi.h"  
#include "../db/dal_destination.h"
#include "../db/dal_cooperative.h"
#include "../db/dal_plage_interdite.h"
#include "../db/dal_configuration.h"
#include "../db/dal_billet.h"
#include "../db/dal_client.h"


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
        bool en_plage_interdite(int temps) const noexcept;

        // --- Données de Persistance ---
        DatabaseManager* m_dbManager = nullptr;
        DalVoiture* m_dalVoiture = nullptr; 
        DalConvoi* m_dalConvoi = nullptr;
        DalClient* m_dalClient = nullptr;
        DalBillet* m_dalBillet = nullptr;

        // --- Contrôle UI ---
        bool m_en_pause = true;
        int m_multiplicateur_vitesse = 1;

    public:
        int m_prochain_id_client; // Public car modifié par l'UI et les générateurs

        Simulateur(int id_origine,
                std::vector<Voiture*>& flotte_globale,
                const std::vector<PlageInterdite>& plages,
                const std::unordered_map<int, int>& durees_trajet,
                Billetterie& billetterie,
                GenerateurDemandes& generateur,
                Planificateur& planificateur,
                int frequence_planif = 30,
                int duree_franchissement = 2,
                DatabaseManager* dbManager = nullptr, 
                DalVoiture* dalVoiture = nullptr,     
                DalConvoi* dalConvoi = nullptr,       
                DalClient* dalClient = nullptr,       
                DalBillet* dalBillet = nullptr
            );
            
            static bool orchestrer_demarrage(
                DatabaseManager& db, 
                std::vector<Voiture>& conteneur_physique, 
                std::vector<Voiture*>& flotte_pointeurs,
                std::vector<Destination>& destinations_ram,
                std::vector<Cooperative>& cooperatives_ram,
                std::vector<PlageInterdite>& plages_ram,
                std::unordered_map<std::string, int>& parametres_ram);

            void tick(int T);
            void executer(int duree_simulation);
            void synchroniser_bdd();
            void enregistrer_embarquement(int id_voiture, int id_destination, int nb_passagers_a_embarquer, double prix_du_billet);
           
            // --- Contrôle du temps (UI) ---
            void set_en_pause(bool pause) { m_en_pause = pause; }
            void set_vitesse(int vitesse) { m_multiplicateur_vitesse = vitesse; }
            bool est_en_pause() const { return m_en_pause; }
            int get_vitesse() const { return m_multiplicateur_vitesse; }
            void avancer_dune_minute();
            void simuler_pas(); 

            // --- Entry Points (Injections manuelles) ---
            void injecter_demande_manuelle(int id_dest, int nb_passagers, bool est_retour, bool est_urgent);
            void ajouter_plage_interdite_ui(int debut, int fin);

            // --- Getters d'Observation (UI) ---
            int get_temps_continu() const { return m_temps_continue; }
            int get_portail_occupe_jusqua() const { return m_portail_occupe_jusqua; }
            Billetterie& get_billetterie() { return m_billetterie; }
            int get_prochain_id_client() const { return m_prochain_id_client; }
            const std::unordered_map<int, int>& get_durees_trajet() const { return m_durees_trajet; }
            const std::vector<Convoi>& get_convois_sortie() const { return m_planificateur.get_convois_sortie(); }
            const std::vector<Convoi>& get_convois_entree() const { return m_planificateur.get_convois_entree(); }
            Planificateur& get_planificateur() { return m_planificateur; }
            const std::vector<PlageInterdite>& get_plages_interdites() const { return m_plages_interdites; }
};

#endif // SIMULATEUR_H