#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "Voiture.h"   // pour Localisation

class Destination {
public:
    Destination(int id, const std::string& nom, Localisation localisation, int duree_trajet);

    int get_id() const;
    std::string get_nom() const;
    Localisation get_localisation() const;
    int get_duree_trajet() const;

private:
    int m_id;
    std::string m_nom;
    Localisation m_localisation;
    int m_duree_trajet;   // en minutes
};

Localisation stringToLocalisation(const std::string& nom) {
    if (nom == "DIEGO") return Localisation::DIEGO;
    if (nom == "MAJUNGA") return Localisation::MAJUNGA;
    if (nom == "TAMATAVE") return Localisation::TAMATAVE;
    if (nom == "AMBATONDRAZAKA") return Localisation::AMBATONDRAZAKA;
    if (nom == "ANTSIRABE") return Localisation::ANTSIRABE;
    if (nom == "FIANARANTSOA") return Localisation::FIANARANTSOA;
    if (nom == "TOLIARA") return Localisation::TOLIARA;
    return Localisation::GARE_PRINCIPAL; // ou throw
}

std::vector<Destination> chargerDestinationsDepuisCSV(const std::string& chemin) {
    std::vector<Destination> destinations;
    std::ifstream fichier(chemin);
    
    if (!fichier.is_open()) {
        throw std::runtime_error("Impossible d'ouvrir " + chemin);
    }

    std::string ligne;
    std::getline(fichier, ligne);   // ignorer l'en-tête

    while (std::getline(fichier, ligne)) {
        if (ligne.empty()) continue;  // ignorer lignes vides

        size_t virgule = ligne.find(',');
        std::string nom = ligne.substr(0, virgule);
        int duree = std::stoi(ligne.substr(virgule + 1));

        Localisation loc = stringToLocalisation(nom);
        int id = static_cast<int>(loc);
        destinations.emplace_back(id, nom, loc, duree);
    }
    return destinations;
}