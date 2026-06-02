#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cassert>

// core 
#include "Configuration.h"
#include "Voiture.h"
#include "Destination.h"
#include "Cooperative.h"
#include "Convoi.h"
#include "PlageInterdite.h"

// simulateur
#include "Planificateur.h"
#include "Billetterie.h"
#include "Generateur.h"

// poor executer .tests/test_plamificateur <-> ctest -R test_planificateur -V
int main() 
{
    std::cout << "===========================================" << std::endl;
    std::cout << "   TEST D'INTEGRATION : GARE ROUTIERE      " << std::endl;
    std::cout << "===========================================" << std::endl;

    // ────────────────────────────────────────────────────────────────
    // ÉTAPE 1 : CHARGEMENT DE LA CONFIGURATION (Fichiers CSV)
    // ────────────────────────────────────────────────────────────────
    Configuration config;
    // directory " requirement" must be in CMAKE_SOURCE_DIR
    std::cout << "[1] Chargement des fichiers CSV..." << std::endl;
    if (!config.charger("requirement")) 
    {
        std::cerr << "ERREUR CRITIQUE : Impossible de charger les donnees CSV. Verifiez le dossier 'requirement/'." << std::endl;
        return 1;
    }
    std::cout << "    -> Succes ! " << config.get_voitures().size() << " voitures chargees." << std::endl;
    // ────────────────────────────────────────────────────────────────
    // ÉTAPE 2 : INITIALISATION DE L'ÉTAT DU MONDE (La flotte active)
    // ────────────────────────────────────────────────────────────────
    std::cout << "[2] Initialisation de la flotte active..." << std::endl;
    
    // On copie les voitures de la config (read-only) vers notre état de simulation (modifiables)
    std::unordered_map<int, Voiture> flotte_active = config.get_voitures();
    
    std::vector<Voiture*> voitures_gare;
    std::unordered_map<int, std::vector<Voiture*>> voitures_province;
    
    // On distribue les pointeurs selon la position actuelle des voitures
    for (auto& paire : flotte_active)
    {
        Voiture* v = &paire.second;

        if (v->get_etat() == EtatVoiture::EN_ATTENTE_GARE) 
        {
            voitures_gare.push_back(v);
        } 
        else if (v->get_etat() == EtatVoiture::EN_ATTENTE_STATION) 
        {
            //destination est une cle |no inserer un cle et son element en meme temps 
            voitures_province[v->get_position()].push_back(v);
        }
    }
    
    std::cout << "    -> " << voitures_gare.size() << " voitures a la gare principale." << std::endl;
    std::cout << "    -> Voitures reparties dans " << voitures_province.size() << " provinces." << std::endl;

    // ────────────────────────────────────────────────────────────────
    // ÉTAPE 3 : INITIALISATION DES MOTEURS DU SIMULATEUR
    // ────────────────────────────────────────────────────────────────
    std::cout << "[3] Demarrage Generateur, Billetterie et Planificateur..." << std::endl;
    
    int capacite_defaut = config.get_parametre("capacite_defaut");
    GenerateurDemandes generateur(flotte_active.size(), capacite_defaut, 42); // Graine 42 pour la reproductibilité
    
    // On ajoute toutes les destinations au générateur avec une populariter  arbitraire
    for (const auto& paire : config.get_destinations()) 
    {
        if (paire.first != 0) { // On ne génère pas de demande vers la gare elle-même , que pour les provinces 
            generateur.ajouter_destination(paire.first, 2.5); // Lambda = 2.5 clients / minute environ
        }
        else
        {
            generateur.ajouter_destination(paire.first, 4.5); // Lambda = 3.5 clients / minute environ
        }
    }
    
    Billetterie billetterie;
    Planificateur planificateur(config.get_destinations(), 
    config.get_cooperatives(),
    config.get_plages(), 
    config.get_parametres());

    
    // ────────────────────────────────────────────────────────────────
    // ÉTAPE 4 : SIMULATION D'UN INSTANT (TICK)
    // ────────────────────────────────────────────────────────────────
    double temps_courant = 480; // Correspond à 08:00 du matin (heure de pointe)
    std::cout << "\n========== DEBUT TICK : " << temps_courant << " (08:00 AM) ==========" << std::endl;

    // A. Génération des demandes
    std::cout << ">> Generateur en action..." << std::endl;
    generateur.generer_flux(temps_courant, billetterie);
    std::cout << "   Charge actuelle Billetterie : " << billetterie.obtenir_charge_actuelle() << " clients." << std::endl;

    // B. Extraction et tri par la billetterie
    std::cout << ">> Billetterie : Tri des urgences..." << std::endl;
    std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
    billetterie.extraire_demandes(temps_courant, dep_std, dep_urg, ret_std, ret_urg);
    
    // Affichage des demandes pour la destination 1 (DIEGO)
    std::cout << "   Demandes pour DIEGO (Dest 1) - Standards: " << dep_std[1] << " | Urgentes: " << dep_urg[1] << std::endl;

    // C. Planification Globale
    std::cout << ">> Planificateur : Calcul de l'agenda..." << std::endl;
    bool succes = planificateur.planifier_global(dep_std, dep_urg, ret_std, ret_urg, 
                                                 voitures_gare, voitures_province, temps_courant);

    // ────────────────────────────────────────────────────────────────
    // ÉTAPE 5 : ANALYSE DES RÉSULTATS
    // ────────────────────────────────────────────────────────────────
    if (succes) {
        const auto& sorties = planificateur.get_convois_sortie();
        const auto& entrees = planificateur.get_convois_entree();
        
        std::cout << "\n[ RESULTATS DE LA PLANIFICATION ]" << std::endl;
        std::cout << "- Convois de SORTIE crees : " << sorties.size() << std::endl;
        for (const Convoi& c : sorties) {
            std::cout << "  > Convoi ID " << c.get_id() 
                      << " | Type: SORTIE"
                      << " | Heure Depart: " << c.get_horaire_prevue() 
                      << " | Voitures: " << c.get_taille() << std::endl;
        }

        std::cout << "- Convois d'ENTREE crees : " << entrees.size() << std::endl;
        for (const Convoi& c : entrees) {
            std::cout << "  > Convoi ID " << c.get_id() 
                      << " | Type: ENTREE"
                      << " | Heure Depart: " << c.get_horaire_prevue() 
                      << " | Voitures: " << c.get_taille() << std::endl;
        }

        // D. Gestion des rejets (Résidus)
        auto residus = planificateur.calculer_demande_residuelle(dep_std /* + dep_urg (fié pour le test) */
                                                                ,ret_std, sorties, entrees);
        billetterie.traiter_demande_residuelle(temps_courant, residus.first, residus.second);
        std::cout << "- Passagers remis en file d'attente (Residus) : " << billetterie.obtenir_charge_actuelle() << std::endl;

    } else {
        std::cerr << "ERREUR : La planification a echoue !" << std::endl;
    }

    std::cout << "===========================================" << std::endl;
    
    return 0;
}