#ifndef SIMULATEUR_H
#define SIMULATEUR_H

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <iostream>

// Inclusion des composants du domaine de simulation
#include "Voiture.h"
#include "Convoi.h"

// Déclarations anticipées (Forward Declarations) pour briser les dépendances cycliques
class Billetterie;
class GenerateurDemandes;
class Planificateur;

class Simulateur {
private:
    // --- Attributs Techniques Obligatoires ---
    int m_temps_courant;                        // T : Temps continu de la simulation (en minutes)
    int m_portail_occupe_jusqua;                // Verrou physique du goulot d'étranglement
    int m_duree_franchissement_par_voiture;     // Fixé à 2 minutes par véhicule
    int m_frequence_planif;                     // Cadencement du cerveau planificateur
    int m_duree_trajet;                         // Temps requis pour relier la gare et une province

    // --- Collections de Gestion de Flotte et d'Agenda ---
    std::vector<Voiture*> m_voitures;           // Registre global de toutes les voitures
    std::vector<Convoi> m_convois_sortie;       // Liste active des convois quittant la gare
    std::vector<Convoi> m_convois_entree;       // Liste active des convois revenant des provinces

    // --- Références aux Composants Métier (Injectés par dépendance) ---
    Billetterie& m_billetterie;
    GenerateurDemandes& m_generateur;
    Planificateur& m_planificateur;

    // --- Méthodes Physiques et Utilitaires Privées ---
    
    /**
     * @brief Valide si le portail est soumis à un couvre-feu ou une restriction de sécurité.
     * @param temps Minute courante à tester.
     * @return true si le portail est inaccessible pour les sorties.
     */
    bool en_plage_interdite(int temps) const noexcept;

    /**
     * @brief Assure la cohérence immédiate de l'état du système avec la source de vérité (SQLite).
     * @param voiture Référence vers le véhicule dont l'état ou la position vient de muter.
     */
    void mettre_a_jour_sqlite(const Voiture& voiture);

public:
    /**
     * @brief Constructeur par injection de dépendances du Simulateur Orchestrateur.
     */
    Simulateur(Billetterie& billetterie, 
               GenerateurDemandes& generateur, 
               Planificateur& planificateur,
               const std::vector<Voiture*>& flotte_initiale,
               int frequence_planif = 30,
               int duree_trajet = 30) noexcept;

    /**
     * @brief Exécute une itération isolée de la simulation correspondant à une minute (Tick).
     * @param T Minute absolue de l'exécution.
     */
    void tick(int T);

    /**
     * @brief Boucle principale pilotant l'incrémentation du temps continu.
     * @param duree_simulation Nombre total de minutes à simuler.
     */
    void executer(int duree_simulation);

    // --- Getters de Monitoring pour les tests et l'observation ---
    int get_temps_courant() const noexcept { return m_temps_courant; }
    int get_portail_occupe_jusqua() const noexcept { return m_portail_occupe_jusqua; }
    const std::vector<Convoi>& get_convois_sortie() const noexcept { return m_convois_sortie; }
    const std::vector<Convoi>& get_convois_entree() const noexcept { return m_convois_entree; }
};

#endif // SIMULATEUR_H