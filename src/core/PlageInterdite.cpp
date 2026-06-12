#include "PlageInterdite.h"
#include <stdexcept>

PlageInterdite::PlageInterdite(int debut, int fin)
    : m_debut(debut), m_fin(fin)
{
    // variant :
    if (debut < 0 || fin < 0 || debut >= fin)
    {
        throw std::invalid_argument(
            "Plage invalide : debut=" + std::to_string(debut) 
            + " fin=" + std::to_string(fin)
        );
    }
}

int PlageInterdite::get_debut() const { return m_debut; }
int PlageInterdite::get_fin() const   { return m_fin; }

int PlageInterdite::get_duree() const {
    return m_fin - m_debut;
}

// ─── VÉRIFICATIONS ────────────────────────────────────────

bool PlageInterdite::contient(int horaire) const {
    // Strictement à l'intérieur (les bornes sont incluses ? 
    // On considère que le franchissement ne peut pas commencer 
    // exactement à la borne, donc on exclut les bornes)
    return horaire > m_debut && horaire < m_fin;
}

// inutilse
bool PlageInterdite::est_trop_proche(int horaire, int marge) const {
    // Vérifie si l'horaire est dans la zone dangereuse autour de la plage
    // Zone dangereuse = [debut - marge, fin + marge]
    return horaire >= (m_debut - marge) && horaire <= (m_fin + marge);
}