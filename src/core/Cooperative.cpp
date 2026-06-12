#include "Cooperative.h"

Cooperative::Cooperative(int id, const std::string& nom)
    : m_id(id), m_nom(nom)
{
}

int Cooperative::get_id() const { return m_id; }
std::string Cooperative::get_nom() const { return m_nom; }

// ─── GESTION DE LA FLOTTE ─────────────────────────────────
void Cooperative::ajouter_voiture(Voiture* v) {
    if (v) {
        m_voitures.push_back(v);
    }
}

void Cooperative::retirer_voiture(int id_voiture) {
    for (auto it = m_voitures.begin(); it != m_voitures.end(); ++it) 
    {
        if ((*it)->get_id() == id_voiture) 
        {
            m_voitures.erase(it);
            return;
        }
    }
}

const std::vector<Voiture*>& Cooperative::get_voitures() const {
    return m_voitures;
}

int Cooperative::get_nb_voitures() const {
    return static_cast<int>(m_voitures.size());
}

// ─── RECHERCHE MÉTIER ─────────────────────────────────────
// A la gare seulement 
Voiture* Cooperative::trouver_voiture_disponible(int id_destination) const {
    for (auto* v : m_voitures) 
    {
        if (v->get_etat() == EtatVoiture::EN_ATTENTE_GARE          // à la gare
            && v->get_destination() == id_destination               // bonne destination
            && !(v->est_pleine()) ) {                                  // places libres
            return v; // la vraie voiture pas une copie
        }
    }
    return nullptr;  // aucune voiture disponible grace au pointeur 
}