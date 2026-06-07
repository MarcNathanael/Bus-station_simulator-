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
#include "Generateur.h"
#include "Billetterie.h"
#include "Simulateur.h"

// ========================================================================
// CONTEXTE GLOBAL : Encapsulation pour éviter les fuites de mémoire
// ========================================================================
struct ContexteSimulateur {
    std::vector<Voiture*> flotte;
    std::unordered_map<int, int> durees_trajet;
    Billetterie billetterie;
    GenerateurDemandes* generateur = nullptr;
    Planificateur* planificateur = nullptr;
    Simulateur* simulateur = nullptr;

    ~ContexteSimulateur() {
        delete simulateur;
        delete planificateur;
        delete generateur;
        for (auto* v : flotte) delete v;
    }
};

// ========================================================================
// UTILITAIRE : Initialisation de l'écosystème complet
// ========================================================================
ContexteSimulateur creer_ecosysteme(Configuration& config) {
    ContexteSimulateur ctx;
    
    // 1. Extraction des durées de trajet
    for (const auto& paire : config.get_destinations()) {
        ctx.durees_trajet[paire.first] = paire.second.get_duree_trajet();
    }

    // 2. Allocation de la flotte
    for (const auto& paire : config.get_voitures()) {
        ctx.flotte.push_back(new Voiture(paire.second));
    }

    // 3. Moteurs
    ctx.generateur = new GenerateurDemandes(ctx.flotte.size(), 40, 42);
    for (const auto& paire : config.get_destinations()) {
        if (paire.first != 0) ctx.generateur->ajouter_destination(paire.first, 2.5);
    }

    ctx.planificateur = new Planificateur(config.get_destinations(), 
                                          config.get_cooperatives(), 
                                          config.get_plages(), 
                                          config.get_parametres());

    // 4. Simulateur
    int freq = 30;
    try { freq = config.get_parametre("frequence_planification"); } catch(...) {}
    
    int franchissement = config.get_parametre("duree_franchissement_voiture");

    ctx.simulateur = new Simulateur(
        0, // ID Origine (Gare Principale)
        ctx.flotte, 
        config.get_plages(), 
        ctx.durees_trajet,
        ctx.billetterie, 
        *ctx.generateur, 
        *ctx.planificateur, 
        freq, 
        franchissement
    );

    return ctx;
}

// ========================================================================
// SCÉNARIO 1 : Le verrou du portail (Phase 6 du Tick)
// Objectif : Vérifier que deux convois prêts ne sortent pas en même temps.
// ========================================================================
void test_sim_verrou_portail(Configuration& config) {
    std::cout << "[TEST SIM] Phase 6 : Le portail filtre les departs simultanes..." << std::endl;
    ContexteSimulateur ctx = creer_ecosysteme(config);

    // On crée 2 convois artificiels prêts à partir à T=10
    Voiture* v1 = ctx.flotte[0];
    Voiture* v2 = ctx.flotte[1];
    v1->set_etat(EtatVoiture::EN_ATTENTE_GARE);
    v2->set_etat(EtatVoiture::EN_ATTENTE_GARE);

    Convoi c1(1, TypeConvoi::SORTIE);
    c1.ajouter_voiture(v1);
    c1.set_horaire_prevue(400);
    c1.set_etat(EtatConvoi::PRET);

    Convoi c2(2, TypeConvoi::SORTIE);
    c2.ajouter_voiture(v2);
    c2.set_horaire_prevue(400);
    c2.set_etat(EtatConvoi::PRET);

    ctx.planificateur->get_convois_sortie().push_back(c1);
    ctx.planificateur->get_convois_sortie().push_back(c2);

    // T = 400 : Le simulateur laisse passer le premier, verrouille le portail
    ctx.simulateur->tick(400);
    
    assert(v1->get_etat() == EtatVoiture::EN_ROUTE); // Le 1er est passé
    assert(v2->get_etat() != EtatVoiture::EN_ROUTE); // Le 2ème est bloqué par m_portail_occupe_jusqua

    // On avance le temps jusqu'à la libération (10 + duree_franchissement)
    int duree_franchissement = config.get_parametre("duree_franchissement_voiture");
    ctx.simulateur->tick(400 + duree_franchissement);

    assert(v2->get_etat() == EtatVoiture::EN_ROUTE); // Le 2ème peut enfin passer

    std::cout << "   -> SUCCES !" << std::endl;
}

// ========================================================================
// SCÉNARIO 2 : Arrivée en Province et Débarquement (Phase 2 du Tick)
// Objectif : Valider qu'une voiture sur la route arrive et se vide.
// ========================================================================
void test_sim_arrivee_province(Configuration& config) {
    std::cout << "[TEST SIM] Phase 2 : Arrivee en province et debarquement..." << std::endl;
    ContexteSimulateur ctx = creer_ecosysteme(config);

    Voiture* v = ctx.flotte[0];
    v->set_etat(EtatVoiture::EN_ROUTE);
    v->set_destination(1); // Province 1
    v->set_heure_arrivee(50.0);
    v->embarquer(20);

    // T = 49 : Pas encore arrivée
    ctx.simulateur->tick(49);
    assert(v->get_etat() == EtatVoiture::EN_ROUTE);
    assert(v->get_places_libres() < v->get_places_max()); // Passagers toujours à bord

    // T = 50 : Arrivée !
    ctx.simulateur->tick(50);
    assert(v->get_etat() == EtatVoiture::EN_ATTENTE_STATION);
    assert(v->get_places_libres() == v->get_places_max()); // Voiture entièrement vidée

    std::cout << "   -> SUCCES !" << std::endl;
}

// ========================================================================
// SCÉNARIO 3 : TEST D'INTÉGRATION GLOBALE (La Boucle Complète)
// Objectif : Faire tourner l'univers entier pendant 12 heures et valider
// que les voitures ont bougé de manière autonome.
// ========================================================================
void test_simulation_integrale(Configuration& config) {
    std::cout << "[TEST SIM] Execution Globale (12 heures de simulation)..." << std::endl;
    ContexteSimulateur ctx = creer_ecosysteme(config);

    // Avant la simulation : Tout le monde est à l'état initial (attente)
    int voitures_initialement_en_route = 0;
    for (auto* v : ctx.flotte) {
        if (v->get_etat() == EtatVoiture::EN_ROUTE) voitures_initialement_en_route++;
    }
    assert(voitures_initialement_en_route == 0);

    // Lancement de l'horloge sur 720 minutes (12 heures)
    ctx.simulateur->executer(720);

    // Après la simulation : Grâce au Générateur et au Planificateur, 
    // l'écosystème doit être vivant. Des voitures doivent être sur la route, 
    // d'autres en province.
    int voitures_en_route = 0;
    int voitures_en_province = 0;

    for (auto* v : ctx.flotte) {
        if (v->get_etat() == EtatVoiture::EN_ROUTE) voitures_en_route++;
        if (v->get_etat() == EtatVoiture::EN_ATTENTE_STATION) voitures_en_province++;
    }

    // On vérifie que le moteur a bien déclenché des départs
    assert(voitures_en_route > 0 || voitures_en_province > 0);

    std::cout << "   -> SUCCES : L'ecosysteme est vivant (" 
              << voitures_en_route << " en route, " 
              << voitures_en_province << " en province)." << std::endl;
}

// ========================================================================
// MAIN : Exécution de la suite
// ========================================================================
int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "   LANCEMENT DES TESTS DU SIMULATEUR       " << std::endl;
    std::cout << "===========================================" << std::endl;

    Configuration config;
    if (!config.charger("requirement")) {
        std::cerr << "ERREUR : Impossible de charger la configuration." << std::endl;
        return 1;
    }

    try {
        test_sim_verrou_portail(config);
        test_sim_arrivee_province(config);
        test_simulation_integrale(config);
        
        std::cout << "\n>>> [100%] TOUS LES TESTS GLOBAUX SONT PASSES ! <<<" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n[ECHEC] Erreur fatale detectee : " << e.what() << std::endl;
        return 1;
    }

    return 0;
}