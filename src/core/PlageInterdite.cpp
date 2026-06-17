#include "PlageInterdite.h"
#include <stdexcept>
#include <string>

PlageInterdite::PlageInterdite(int debut, int fin)
    : m_debut(debut), m_fin(fin)
{
    // On vérifie simplement que les minutes sont valides dans une journée (0 à 1439)
    if (debut < 0 || debut >= 1440 || fin < 0 || fin >= 1440)
    {
        throw std::invalid_argument(
            "Plage invalide : les valeurs doivent être entre 0 et 1439. Reçu: debut=" 
            + std::to_string(debut) + " fin=" + std::to_string(fin)
        );
    }
    
    if (debut == fin) {
        throw std::invalid_argument("Le début et la fin d'une plage ne peuvent pas être identiques.");
    }
}

int PlageInterdite::get_debut() const { return m_debut; }
int PlageInterdite::get_fin() const   { return m_fin; }

int PlageInterdite::get_duree() const {
    if (m_debut < m_fin) {
        // Cas normal (ex: 14h à 16h)
        return m_fin - m_debut;
    } else 
    {
        // Cas à cheval sur minuit (ex: 23h à 1h -> (1440 - 1380) + 60 = 120 min)
        return (1440 - m_debut) + m_fin;
    }
}

// L'unique source de vérité pour savoir si une minute est interdite
bool PlageInterdite::contient(int horaire_absolu) const {
    // On applique le modulo 1440 ici pour sécuriser l'appel avec un temps absolu (ticks cumulés)
    int heure_circulaire = horaire_absolu % 1440;

    if (m_debut < m_fin) {
        // Plage normale dans la même journée
        return heure_circulaire >= m_debut && heure_circulaire < m_fin;
    } else {
        // Plage chevauchant minuit (ex: >= 23h OU < 01h)
        return heure_circulaire >= m_debut || heure_circulaire < m_fin;
    }
}