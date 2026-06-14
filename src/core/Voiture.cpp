#include "Voiture.h"

// ─── Constructeur simple ──────────────────────────────────
Voiture::Voiture(int id, int places_max, int destination_initiale)
    : m_id(id)
    , m_nb_places_max(places_max)
    , m_nb_places_libres(places_max) // Au départ, toutes les places sont vides
    , m_id_destination(destination_initiale)
    , m_id_position(0)               // Commence par défaut à la Gare Principale (0)
    , m_etat(EtatVoiture::EN_ATTENTE_GARE)
    , m_heure_arrivee(-1.0)       // Initialisation sécurisée à -1 (pas sur la route)
    , m_suivant(nullptr)
    , m_est_modifie(false) // <--- ICI
{
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
    m_suivant(nullptr),
    m_est_modifie(false)
{
}

// ─── Gestion du Dirty Bit ───────────────────────────────────
bool Voiture::is_dirty() const { return m_est_modifie; }
void Voiture::clear_dirty() { m_est_modifie = false; }

// ─── Getters ──────────────────────────────────────────────
int Voiture::get_id() const { return m_id; }
int Voiture::get_id_coop() const { return m_id_coop; }
int Voiture::get_places_libres() const { return m_nb_places_libres; }
int Voiture::get_places_max() const { return m_nb_places_max; }
int Voiture::get_horaire_depart() const { return m_horaire_depart; }
EtatVoiture Voiture::get_etat() const { return m_etat; }
int Voiture::get_position() const { return m_id_position; }
int Voiture::get_destination() const { return m_id_destination; }
double Voiture::get_heure_arrivee() const { return m_heure_arrivee; }
int Voiture::get_passagers() const { return m_nb_places_max - m_nb_places_libres; }

bool Voiture::embarquer(int nb_passagers) {
    if (nb_passagers <= 0 || nb_passagers > m_nb_places_libres) {
        return false;
    }
    m_nb_places_libres -= nb_passagers;
    m_est_modifie = true; 
    return true;
}

bool Voiture::debarquer(int nb_passagers) {
    int passagers_actuels = m_nb_places_max - m_nb_places_libres;
    if (nb_passagers <= 0 || nb_passagers > passagers_actuels) {
        return false;
    }
    m_nb_places_libres += nb_passagers;
    m_est_modifie = true; 
    return true;
}
void Voiture::debarquer_tous() {
    m_nb_places_libres = m_nb_places_max; // Libère l'intégralité des sièges d'un coup
    m_est_modifie = true; 

}

bool Voiture::est_pleine() const 
{
    return m_nb_places_libres == 0;
}

// ─── Setters ─────────────────────────────────────────────
void Voiture::set_etat(EtatVoiture etat) {
    m_etat = etat;
    
    // Logique physique automatique associée aux transitions d'état :
    if (m_etat == EtatVoiture::EN_ATTENTE_GARE) {
        m_id_position = 0;          // Physiquement présente à la gare
        m_heure_arrivee = -1.0;  // Statique, le compteur d'arrivée est réinitialisé
    } 
    else if (m_etat == EtatVoiture::EN_ATTENTE_STATION) {
        m_id_position = m_id_destination; // Physiquement arrivée dans sa province
        m_heure_arrivee = -1.0;     // Statique
    }
    m_est_modifie = true; 

}

void Voiture::set_position(int pos) { m_id_position = pos; m_est_modifie = true;}
void Voiture::set_horaire_depart(int minutes) { m_horaire_depart = minutes; m_est_modifie = true;}
void Voiture::set_destination(int id_dest) { m_id_destination = id_dest; m_est_modifie = true;}
void Voiture::set_heure_arrivee(double heure) { m_heure_arrivee = heure; m_est_modifie = true;}
