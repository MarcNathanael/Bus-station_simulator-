#include "Destination.h"

Destination::Destination(int id, const std::string& nom, int duree_trajet, float posX, float posY)
    : m_id(id), 
      m_nom(nom),
      m_duree_trajet(duree_trajet),
      m_positionX(posX),
      m_positionY(posY)
{}

int Destination::get_id() const { return m_id; }
std::string Destination::get_nom() const { return m_nom; }
int Destination::get_duree_trajet() const { return m_duree_trajet; }

float Destination::get_positionX() const { return m_positionX; }
float Destination::get_positionY() const { return m_positionY; }