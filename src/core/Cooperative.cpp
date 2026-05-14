#include "Cooperative.h"

Cooperative::Cooperative(int id, const std::string& nom)
    : m_id(id), m_nom(nom) {}

int Cooperative::get_id() const 
{ 
    return m_id; 
}
std::string Cooperative::get_nom() const 
{ 
    return m_nom; 
}