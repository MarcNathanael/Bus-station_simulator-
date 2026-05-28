#pragma once
#include "structures.h"
#include "Billetterie.h"
#include <vector>
#include <unordered_map>
#include <random>

class GenerateurDemandes {
public:
    // Initialise le générateur avec le plafond physique de la gare
    // la graine sert a varier les demandes tout en genererant le meme a chaque tour de minute
    GenerateurDemandes(int nb_total_voitures, int places_par_voiture, int graine = 42);

    void ajouter_destination(int id_dest, double lambda_base);

    // Enregistre les arrivées en province pour générer le trafic de retour plus tard
    void enregistrer_arrivee_province(int id_province, int nb_passagers, int temps_courant);

    // Fonction principale : génère les flux et les envoie à la Billetterie
    void generer_flux(int temps_courant, Billetterie& billetterie);

private:
    // objet moteur de hazard
    std::mt19937 m_generateur;
    int m_plafond_gare; 
    std::unordered_map<int, double> m_lambdas_base;// sert a simuler la populariter d'un destination 

    // File des voyageurs en train de séjourner en province
    struct Sejour {
        int id_province;
        int nb_passagers;
        int temps_fin_sejour; // quand ce temps est ecouler , il rentre a la gare
    };
    std::vector<Sejour> m_incubateur_province;

    // c'est pour le lamdba de la loi de poisson
    double obtenir_facteur_horaire(int temps_courant, bool est_un_retour) const;
};