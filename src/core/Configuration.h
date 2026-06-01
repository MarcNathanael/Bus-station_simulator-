#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Destination.h"
#include "Cooperative.h"
#include "Voiture.h"
#include "PlageInterdite.h"

class Configuration {
public:
    // Charge tous les CSV depuis un dossier (ex: "data/")
    bool charger(const std::string& dossier);

    // Accès aux données chargées et les attribur sont imodifiable
    const std::unordered_map<int, Destination>& get_destinations() const;
    const std::unordered_map<int, Cooperative>& get_cooperatives() const;
    const std::unordered_map<int, Voiture>& get_voitures() const;
    const std::vector<PlageInterdite>& get_plages() const;
    const std::unordered_map<std::string, int>& get_parametres() const;
    int get_parametre(const std::string& cle) const;

private:
    // Fonctions de parsing individuelles
    void parser_destinations(const std::string& chemin);
    void parser_cooperatives(const std::string& chemin);
    void parser_voitures(const std::string& chemin);
    void parser_plages(const std::string& chemin);
    void parser_parametres(const std::string& chemin);

    std::unordered_map<int, Destination> m_destinations;
    std::unordered_map<int, Cooperative> m_cooperatives;
    std::unordered_map<int, Voiture> m_voitures;
    std::vector<PlageInterdite> m_plages;
    std::unordered_map<std::string, int> m_parametres;
};