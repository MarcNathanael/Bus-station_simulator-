#include "PlageInterdite.h"

PlageInterdite::PlageInterdite(int debut, int fin)
    : m_debut(debut), m_fin(fin) {}

int PlageInterdite::get_debut() const 
{ 
    return m_debut; 
}
int PlageInterdite::get_fin() const 
{ 
    return m_fin; 
}