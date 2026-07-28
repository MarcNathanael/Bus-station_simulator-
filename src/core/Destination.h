#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "Voiture.h"   // pour Localisation

class Destination {
public:
    // Ajout de posX et posY dans le constructeur
    Destination(int id, const std::string& nom, int duree_trajet, float posX, float posY);

    int get_id() const;
    std::string get_nom() const;
    int get_duree_trajet() const;
    
    // Nouveaux getters pour l'UI
    float get_positionX() const;
    float get_positionY() const;

private:
    int m_id;
    std::string m_nom;
    int m_duree_trajet;
    float m_positionX;
    float m_positionY;
};