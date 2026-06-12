#pragma once
#include <string>
#include <vector>
#include "Voiture.h"

class Cooperative {
public:
    Cooperative(int id, const std::string& nom);

    int get_id() const;
    std::string get_nom() const;

    // Gestion de la flotte 
    void ajouter_voiture(Voiture* v);

    // inutile
    void retirer_voiture(int id_voiture);
    const std::vector<Voiture*>& get_voitures() const;
    int get_nb_voitures() const;

    // Recherche métier
    Voiture* trouver_voiture_disponible(int id_destination) const;
    // Retourne une voiture EN_ATTENTE_GARE, avec places libres, 
    // assignée à cette destination. nullptr si aucune.

private:
    int m_id;
    std::string m_nom;
    std::vector<Voiture*> m_voitures; // evite la copie
};