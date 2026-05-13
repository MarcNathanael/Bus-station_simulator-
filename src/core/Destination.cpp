#include "Destination.h"

Destination::Destination(int id, const std::string& nom, Localisation localisation, int duree_trajet)
    : m_id(id)
    , m_nom(nom)
    , m_localisation(localisation)
    , m_duree_trajet(duree_trajet)
{
}

int Destination::get_id() const { return m_id; }
std::string Destination::get_nom() const { return m_nom; }
Localisation Destination::get_localisation() const { return m_localisation; }
int Destination::get_duree_trajet() const { return m_duree_trajet; }