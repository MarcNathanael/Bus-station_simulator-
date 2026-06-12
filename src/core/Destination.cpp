#include "Destination.h"

Destination::Destination(int id, const std::string& nom, int duree_trajet)
    : m_id(id), 
      m_nom(nom),
      m_duree_trajet(duree_trajet)
{
}

//pas besoin de setters les destinations sont static

int Destination::get_id() const 
{ 
    return m_id; 
}
std::string Destination::get_nom() const 
{ 
    return m_nom; 
}

int Destination::get_duree_trajet() const 
{ 
    return m_duree_trajet; 
}