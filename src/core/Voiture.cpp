#include "Voiture.h"

// ─── Constructeur simple ──────────────────────────────────
Voiture::Voiture(int id, int id_coop, int id_destination)
    : Voiture(id, id_coop, id_destination,
        0, // position = GARE_PRINCIPAL (ID 0)
        32, // capacité max
        32, // places libres
        EtatVoiture::EN_ATTENTE_GARE,
        -1) // horaire non planifié
{
    // Le corps est vide
}

// ─── Constructeur complet ─────────────────────────────────
Voiture::Voiture(int id, int id_coop, int id_destination, int id_position,
                 int capacite_max, int places_libres, EtatVoiture etat, int horaire_depart)
    : m_id(id),
    m_id_coop(id_coop),
    m_nb_places_libres(places_libres),
    m_nb_places_max(capacite_max),
    m_horaire_depart(horaire_depart),
    m_etat(etat),
    m_id_position(id_position),
    m_id_destination(id_destination),
    m_suivant(nullptr)
{
}

// ─── Getters ──────────────────────────────────────────────
int Voiture::get_id() const { return m_id; }
int Voiture::get_id_coop() const { return m_id_coop; }
int Voiture::get_places_libres() const { return m_nb_places_libres; }
int Voiture::get_places_max() const { return m_nb_places_max; }
int Voiture::get_horaire_depart() const { return m_horaire_depart; }
EtatVoiture Voiture::get_etat() const { return m_etat; }
int Voiture::get_position() const { return m_id_position; }
int Voiture::get_destination() const { return m_id_destination; }

// ─── Actions métier ──────────────────────────────────────
bool Voiture::embarquer(int nb_passagers) 
{
    if (nb_passagers <= 0) return false;
    if (m_nb_places_libres >= nb_passagers) 
    {
        m_nb_places_libres -= nb_passagers;
        return true;
    }
    return false;
}

bool Voiture::est_pleine() const 
{
    return m_nb_places_libres == 0;
}

// ─── Setters ─────────────────────────────────────────────
void Voiture::set_etat(EtatVoiture etat) { m_etat = etat; }
void Voiture::set_position(int pos) { m_id_position = pos; }
void Voiture::set_horaire_depart(int minutes) { m_horaire_depart = minutes; }
void Voiture::set_destination(int id_dest) { m_id_destination = id_dest; }
