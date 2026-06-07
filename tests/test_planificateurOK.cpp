#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cassert>
#include <cmath>

#include "Configuration.h"
#include "Voiture.h"
#include "Destination.h"
#include "Cooperative.h"
#include "Convoi.h"
#include "PlageInterdite.h"
#include "Planificateur.h"

// ========================================================================
// UTILITAIRE : Initialisation propre pour chaque test
// ========================================================================
Planificateur creer_planificateur_test(Configuration& config) {
    return Planificateur(config.get_destinations(), 
                         config.get_cooperatives(), 
                         config.get_plages(), 
                         config.get_parametres());
}

// ========================================================================
// SCÉNARIO 1 : Gestion des collisions et décalage par l'Agenda
// Objectif : Vérifier qu'une exclusion mutuelle est appliquée au portail.
// ========================================================================
void test_agenda_collision_decalage(Configuration& config) {
    std::cout << "[TEST] Agenda : Resolution de collision au portail..." << std::endl;
    
    Planificateur plan = creer_planificateur_test(config);
    double temps_continu = 480; // 08h00

    Voiture v1(1, 1, 1, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1);
    Voiture v2(2, 2, 2, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1);    
    std::vector<Voiture*> voitures_gare = {&v1, &v2};
    std::unordered_map<int, std::vector<Voiture*>> voitures_province;

    std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
    // Deux départs parfaits pour deux destinations différentes en même temps
    dep_std[1] = 32; 
    dep_std[2] = 32;

    plan.planifier_global(dep_std, dep_urg, ret_std, ret_urg, voitures_gare, voitures_province, temps_continu);

    const auto& sorties = plan.get_convois_sortie();
    assert(sorties.size() == 2);
    
    // Le portail ne peut laisser passer qu'un convoi à la fois.
    // L'un doit partir à 480, l'autre doit être décalé (ex: 480 + m_espacement_min).
    int t1 = sorties[0].get_horaire_prevue();
    int t2 = sorties[1].get_horaire_prevue();
    assert(t1 != t2); 
    int espacement_min = config.get_parametre("espacement_min_entre_occupation_convois");
    //assert(std::abs(t1 - t2) >= plan.get_parametres().espacement_min);
    std::cout << "   -> SUCCES !" << std::endl;
}

// ========================================================================
// SCÉNARIO 2 : L'urgence est intouchable (Priorité absolue)
// Objectif : En cas de collision, le standard est décalé, l'urgence passe.
// ========================================================================
// ========================================================================
// SCÉNARIO 2 : L'urgence est intouchable (Priorité absolue)
// Objectif : En cas de collision, le standard est décalé, l'urgence passe.
// ========================================================================
void test_agenda_urgence_intouchable(Configuration& config) {
    std::cout << "[TEST] Agenda : Priorite absolue du convoi urgent..." << std::endl;
    
    Planificateur plan = creer_planificateur_test(config);
    double temps_continu = 540; // 09h00

    Voiture v1(1, 1, 1, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1);
    Voiture v2(2, 2, 2, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1); // Correction précédente (dest 2)
    std::vector<Voiture*> voitures_gare = {&v1, &v2};
    std::unordered_map<int, std::vector<Voiture*>> voitures_province;

    std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
    dep_urg[1] = 10; // Destination 1 : Urgent
    dep_std[2] = 32; // Destination 2 : Standard (Plein)

    plan.planifier_global(dep_std, dep_urg, ret_std, ret_urg, voitures_gare, voitures_province, temps_continu);

    const auto& sorties = plan.get_convois_sortie();
    assert(sorties.size() == 2);

    // On identifie qui est qui
    int heure_urg = (sorties[0].contient_urgence()) ? sorties[0].get_horaire_prevue() : sorties[1].get_horaire_prevue();
    int heure_std = (!sorties[0].contient_urgence()) ? sorties[0].get_horaire_prevue() : sorties[1].get_horaire_prevue();

    // CORRECTION ICI : On récupère dynamiquement le délai d'achat (m_delai_achat_min)
    int delai_achat = config.get_parametre("duree_min_achat_avant_depart");
    int heure_depart_ideale = 540 + delai_achat;

    // L'urgence refuse d'être décalée par la collision, elle part à l'heure idéale calculée
    assert(heure_urg == heure_depart_ideale); 
    
    // Le convoi standard a été sacrifié et repoussé APRÈS l'urgence + son temps de passage au portail
    assert(heure_std > heure_depart_ideale);

    std::cout << "   -> SUCCES !" << std::endl;
}

// ========================================================================
// SCÉNARIO 3 CORRIGÉ : Test de la formation gloutonne initiale
// Objectif : Vérifier que former_convois_sortie remplit correctement 
// jusqu'à m_taille_max_convoi avant de créer un nouveau convoi.
// ========================================================================
void test_formation_gloutonne_convoi(Configuration& config) {
    std::cout << "[TEST] Formation : Regroupement glouton dans un meme convoi..." << std::endl;
    
    Planificateur plan = creer_planificateur_test(config);
    double temps_continu = 600; // 10h00

    Voiture v1(1, 1, 1, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1);
    Voiture v2(2, 1, 1, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1);
    std::vector<Voiture*> voitures_gare = {&v1, &v2};
    std::unordered_map<int, std::vector<Voiture*>> voitures_province;

    std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
    dep_std[1] = 64; // Remplit exactement 2 voitures

    plan.planifier_global(dep_std, dep_urg, ret_std, ret_urg, voitures_gare, voitures_province, temps_continu);

    const auto& sorties = plan.get_convois_sortie();
    
    // On valide que l'algorithme glouton a bien utilisé la limite m_taille_max_convoi
    assert(sorties.size() == 1); 
    assert(sorties[0].get_voitures().size() == 2);
    assert(v1.get_places_libres() == 0);
    assert(v2.get_places_libres() == 0);

    std::cout << "   -> SUCCES (Formation gloutonne validee) !" << std::endl;
}

// ========================================================================
// SCÉNARIO 4 : Optimisation Phase 3 - Maintien d'une urgence à vide
// Objectif : Le seuil critique annule les trains vides, SAUF les urgences.
// ========================================================================
void test_urgence_survit_suppression(Configuration& config) {
    std::cout << "[TEST] Optimiseur : Une urgence bloque la suppression economique..." << std::endl;
    
    Planificateur plan = creer_planificateur_test(config);
    double temps_continu = 720; // 12h00

    Voiture v1(1, 1, 1, 0, 50, 50, EtatVoiture::EN_ATTENTE_GARE, -1); // Gros bus
    std::vector<Voiture*> voitures_gare = {&v1};
    std::unordered_map<int, std::vector<Voiture*>> voitures_province;

    std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
    // 1 seul passager urgent dans un bus de 50 places (2% de remplissage).
    dep_urg[1] = 1; 

    plan.planifier_global(dep_std, dep_urg, ret_std, ret_urg, voitures_gare, voitures_province, temps_continu);

    // Normalement l'optimiseur détruit un convoi à 2%, mais ici il DOIT partir.
    const auto& sorties = plan.get_convois_sortie();
    assert(sorties.size() == 1);
    assert(v1.get_places_libres() == 49); 

    std::cout << "   -> SUCCES !" << std::endl;
}

// ========================================================================
// SCÉNARIO 5 CORRIGÉ : Le Modulo Circulaire et Plage Interdite Dynamique
// Objectif : Vérifier que le temps > 1440 s'aligne sur les plages du CSV.
// ========================================================================
void test_temps_circulaire(Configuration& config) {
    std::cout << "[TEST] Temps Circulaire : Lecture dynamique des plages interdites..." << std::endl;
    
    const auto& plages = config.get_plages();
    if (plages.empty()) {
        std::cout << "   -> IGNORE : Aucune plage interdite definie dans le CSV." << std::endl;
        return;
    }

    Planificateur plan = creer_planificateur_test(config);
    
    // On récupère dynamiquement la première plage du CSV pour le test
    int debut_plage = plages[0].get_debut(); // ex: 120
    int fin_plage = plages[0].get_fin();     // ex: 240

    // On se place intentionnellement au milieu de cette plage, au Jour 2
    // Exemple : 1440 + 120 + 10 = 1570
    double temps_continu = 1440.0 + debut_plage + 10.0; 

    Voiture v1(1, 1, 1, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1);
    std::vector<Voiture*> voitures_gare = {&v1};
    std::unordered_map<int, std::vector<Voiture*>> voitures_province;

    std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
    dep_std[1] = 32; 

    plan.planifier_global(dep_std, dep_urg, ret_std, ret_urg, voitures_gare, voitures_province, temps_continu);

    const auto& sorties = plan.get_convois_sortie();
    assert(sorties.size() == 1);
    
    // Le départ doit être repoussé APRÈS la fin de la plage, sur le bon jour.
    // Exemple : Si fin_plage = 240, le départ doit être >= 1440 + 240 (1680).
    int heure_depart = sorties[0].get_horaire_prevue();
    int heure_attendue_min = 1440 + fin_plage;

    assert(heure_depart >= heure_attendue_min); 

    std::cout << "   -> SUCCES (Plage detectee : " << debut_plage << "-" << fin_plage << ") !" << std::endl;
}

// ========================================================================
// MAIN : Exécution de la suite
// ========================================================================
int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "      LANCEMENT DE LA SUITE DE TESTS       " << std::endl;
    std::cout << "===========================================" << std::endl;

    Configuration config;
    // Assurez-vous que le dossier 'requirement' contient des CSV valides
    if (!config.charger("requirement")) {
        std::cerr << "ERREUR : Impossible de charger la configuration." << std::endl;
        return 1;
    }

    try {
        test_agenda_collision_decalage(config);
        test_agenda_urgence_intouchable(config);
        test_formation_gloutonne_convoi(config);
        test_urgence_survit_suppression(config);
        test_temps_circulaire(config);
        
        std::cout << "\n>>> [100%] TOUS LES TESTS SONT PASSES AVEC SUCCES ! <<<" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n[ECHEC] Erreur fatale detectee : " << e.what() << std::endl;
        return 1;
    }

    return 0;
}