#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cassert>

#include "Configuration.h"
#include "Voiture.h"
#include "Destination.h"
#include "Cooperative.h"
#include "Convoi.h"
#include "PlageInterdite.h"
#include "Planificateur.h"

// ========================================================================
// FONCTION UTILITAIRE : Prépare une gare vierge pour chaque test
// ========================================================================
Planificateur creer_planificateur_test(Configuration& config) {
    return Planificateur(config.get_destinations(), 
                         config.get_cooperatives(), 
                         config.get_plages(), 
                         config.get_parametres());
}

// ========================================================================
// SCÉNARIO 1 : Le comportement d'urgence outrepasse la rentabilité
// ========================================================================
void test_urgence_ignore_seuil(Configuration& config) {
    std::cout << "[TEST] Lancement : Urgence ignore le seuil de remplissage..." << std::endl;
    
    Planificateur planificateur = creer_planificateur_test(config);
    double temps_courant = 600; // 10h00

    // On prépare une voiture vide de 32 places
    Voiture v1(1, 1, 1, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1);
    std::vector<Voiture*> voitures_gare = {&v1};
    std::unordered_map<int, std::vector<Voiture*>> voitures_province;

    // SCÉNARIO : 2 passagers seulement, mais ils sont URGENTS. 
    // Le taux de remplissage sera de 2/32 (6%), bien en dessous des 70% requis.
    std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
    dep_urg[1] = 2; // 2 urgents pour la destination 1

    bool succes = planificateur.planifier_global(dep_std, dep_urg, ret_std, ret_urg, 
                                                 voitures_gare, voitures_province, temps_courant);

    // VÉRIFICATIONS (Le test plante si une condition est fausse)
    assert(succes == true);
    assert(planificateur.get_convois_sortie().size() == 1); // Le train DOIT partir
    assert(v1.get_places_libres() == 30); // 2 places doivent être prises

    std::cout << "   -> SUCCES !" << std::endl;
}

// ========================================================================
// SCÉNARIO 2 : Respect strict des plages interdites (Travaux)
// ========================================================================
void test_plage_interdite_et_nuit(Configuration& config) {
    std::cout << "[TEST] Lancement : Respect des plages de travaux..." << std::endl;
    
    Planificateur planificateur = creer_planificateur_test(config);
    
    // On simule une heure en plein milieu de la nuit (ex: 01h00 du matin, soit 60 minutes)
    // Dans tes CSV, la plage 0 à 360 (minuit à 6h) est interdite !
    double temps_courant = 60; 

    Voiture v1(2, 1, 1, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1);
    std::vector<Voiture*> voitures_gare = {&v1};
    std::unordered_map<int, std::vector<Voiture*>> voitures_province;

    std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
    dep_std[1] = 32; // Un train plein de 32 passagers standards

    planificateur.planifier_global(dep_std, dep_urg, ret_std, ret_urg, 
                                   voitures_gare, voitures_province, temps_courant);

    // VÉRIFICATION : Le convoi a bien été créé, MAIS son heure de départ doit 
    // être repoussée APRÈS la fin des travaux (donc après 360).
    const auto& sorties = planificateur.get_convois_sortie();
    assert(sorties.size() == 1);
    
    int heure_depart = sorties[0].get_horaire_prevue();
    assert(heure_depart >= 360); // Le train attend sagement l'ouverture de la gare

    std::cout << "   -> SUCCES !" << std::endl;
}

// ========================================================================
// SCÉNARIO 3 : Protection contre le clonage (Passagers fantômes)
// ========================================================================
void test_suppression_convoi_fantome(Configuration& config) {
    std::cout << "[TEST] Lancement : Debarquement correct lors d'une annulation..." << std::endl;
    
    Planificateur planificateur = creer_planificateur_test(config);
    double temps_courant = 500;

    Voiture v1(3, 1, 1, 0, 32, 32, EtatVoiture::EN_ATTENTE_GARE, -1);
    std::vector<Voiture*> voitures_gare = {&v1};
    std::unordered_map<int, std::vector<Voiture*>> voitures_province;

    // SCÉNARIO : Seulement 2 passagers standards. 
    // Le convoi va se former, mais l'Optimiseur (Étape 3) va le supprimer car < seuil_critique.
    std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
    dep_std[1] = 2; 

    planificateur.planifier_global(dep_std, dep_urg, ret_std, ret_urg, 
                                   voitures_gare, voitures_province, temps_courant);

    // VÉRIFICATIONS
    assert(planificateur.get_convois_sortie().size() == 0); // L'optimiseur a bien détruit le convoi
    assert(v1.get_places_libres() == 32); // LES PASSAGERS ONT BIEN ÉTÉ DÉBARQUÉS (Correction du bug)

    std::cout << "   -> SUCCES !" << std::endl;
}

// ========================================================================
// MAIN : Chef d'orchestre des tests
// ========================================================================
int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "      LANCEMENT DE LA SUITE DE TESTS       " << std::endl;
    std::cout << "===========================================" << std::endl;

    Configuration config;
    if (!config.charger("requirement")) {
        std::cerr << "ERREUR : Impossible de charger les donnees CSV." << std::endl;
        return 1;
    }

    try {
        test_urgence_ignore_seuil(config);
        test_plage_interdite_et_nuit(config);
        test_suppression_convoi_fantome(config);
        
        std::cout << "\n>>> TOUS LES TESTS SONT PASSES AVEC SUCCES ! <<<" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n[ECHEC] Un test a provoque une erreur fatale : " << e.what() << std::endl;
        return 1;
    }

    return 0;
}