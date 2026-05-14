#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "Voiture.h"   // pour Localisation

class Destination {
public:
    Destination(int id, const std::string& nom, int duree_trajet);

    int get_id() const;
    std::string get_nom() const;
    int get_duree_trajet() const;

private:
    int m_id;
    std::string m_nom;
    int m_duree_trajet;   // en minutes
};
