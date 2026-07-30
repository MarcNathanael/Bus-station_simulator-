#include "Voiture.h"

// ─── Constructeur simple ──────────────────────────────────
Voiture::Voiture(int id, int places_max, int destination_initiale, int t_charge, int t_decharge)
    : m_id(id)
    , m_id_coop(0)
    , m_id_destination(destination_initiale)
    , m_id_position(0)              
    , m_nb_places_max(places_max)
    , m_nb_places_libres(places_max) 
    , m_horaire_depart(0)
    , m_etat(EtatVoiture::EN_ATTENTE_GARE)
    , m_heure_arrivee(-1.0)       
    , m_est_modifie(false)
    , m_temps_chargement(t_charge)     // Stockage du paramètre CSV
    , m_temps_dechargement(t_decharge) // Stockage du paramètre CSV
{
}

// ─── Constructeur complet ─────────────────────────────────
Voiture::Voiture(int id, int id_coop, int id_destination, int id_position,
                 int capacite_max, int places_libres, EtatVoiture etat, int horaire_depart,
                 int t_charge, int t_decharge)
    : m_id(id),
    m_id_coop(id_coop),
    m_id_destination(id_destination),
    m_id_position(id_position),
    m_nb_places_max(capacite_max),
    m_nb_places_libres(places_libres),
    m_horaire_depart(horaire_depart),
    m_etat(etat),
    m_heure_arrivee(-1.0),
    m_est_modifie(false),
    m_temps_chargement(t_charge),     // Stockage du paramètre CSV
    m_temps_dechargement(t_decharge) // Stockage du paramètre CSV
{
}

// Vos méthodes embarquer et debarquer restent simples et lisibles :
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
    
    // CORRECTION CRITIQUE : On ne modifie plus m_id_destination ici.
    // La destination représente la position physique de la voiture, pas son intention de voyage.
    // Elle ne doit être changée que par le Planificateur ou le Simulateur lors d'un nouveau trajé.
    
    m_est_modifie = true; 
    return true;
}

void Voiture::debarquer_tous() {
    m_nb_places_libres = m_nb_places_max; 
    
    // CORRECTION CRITIQUE : Idem, on préserve la destination (qui est la position physique).
    
    m_est_modifie = true; 
}

// ... reste des méthodes (set_etat, etc.)
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
