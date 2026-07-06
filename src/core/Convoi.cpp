#include "Convoi.h"


Convoi::Convoi(int id, TypeConvoi type, int taille_max)
    : m_id(id), 
      m_type(type), 
      m_etat(EtatConvoi::EN_FORMATION), 
      m_horaire_prevue(-1),
      m_contient_urgence(false), 
      m_id_region(0),
      m_est_modifie(false),
      m_taille_max(taille_max) // On stocke la configuration reçue !
{
}

int Convoi::get_taille_max() const { 
    return m_taille_max; // Plus de constante en dur !
}

bool Convoi::est_plein() const {
    return m_voitures.size() >= static_cast<size_t>(m_taille_max);
}

void Convoi::set_contient_urgence(bool valeur) { 
    if (m_contient_urgence != valeur) {
        m_contient_urgence = valeur; 
    }
    m_est_modifie = true;
}
bool Convoi::contient_urgence() const { return m_contient_urgence; }

// ─── Gestion du Dirty Bit ───────────────────────────────────
bool Convoi::is_dirty() const { return m_est_modifie; }
void Convoi::clear_dirty() { m_est_modifie = false; }

int Convoi::get_id_region() const 
{
        return m_id_region;
}
int Convoi::get_id() const { return m_id; }
TypeConvoi Convoi::get_type() const { return m_type; }
EtatConvoi Convoi::get_etat() const { return m_etat; }
int Convoi::get_taille() const { return static_cast<int>(m_voitures.size()); }
int Convoi::get_horaire_prevue() const { return m_horaire_prevue; }
const std::vector<Voiture*>& Convoi::get_voitures() const { return m_voitures; }

bool Convoi::voiture_est_disponible(const Voiture* v) const // ← vérification
{
    if (!v) return false;
    
    if (m_type == TypeConvoi::SORTIE) {
        // Pour un convoi sortant, la voiture doit être en attente à la gare
        return v->get_etat() == EtatVoiture::EN_ATTENTE_GARE;
    } else {
        // Pour un convoi entrant, la voiture doit être en attente à la station
        return v->get_etat() == EtatVoiture::EN_ATTENTE_STATION;
    }
}

// ─── Gestion des Voitures (Impacte l'état du convoi) ────────
bool Convoi::ajouter_voiture(Voiture* v) {
    if (!v) return false;
    if (m_etat != EtatConvoi::EN_FORMATION) return false; 
    if (est_plein()) return false;
    if (!voiture_est_disponible(v)) return false; 

    // Validation et transition d'état de la voiture
    // (Ceci va automatiquement activer le Dirty Bit de la VOITURE)
    v->set_etat(EtatVoiture::EN_CHARGEMENT);

    m_voitures.push_back(v);
    m_est_modifie = true; // Le convoi a changé (nouvelle voiture)

    return true;
}

// ─── RETIRER UNE VOITURE (modifié) ────────────────────────
bool Convoi::retirer_voiture(int id_voiture) {
    for (auto it = m_voitures.begin(); it != m_voitures.end(); ++it) {
        if ((*it)->get_id() == id_voiture) {
            // (Ceci va automatiquement activer le Dirty Bit de la VOITURE)
            if (m_type == TypeConvoi::SORTIE) {
                (*it)->set_etat(EtatVoiture::EN_ATTENTE_GARE);
            } else {
                (*it)->set_etat(EtatVoiture::EN_ATTENTE_STATION);
            }
            m_voitures.erase(it);
            m_est_modifie = true; // Le convoi a changé (voiture en moins)
            return true;
        }
    }
    return false;
}



void Convoi::set_id_region(int id_region) noexcept {
    if (m_id_region != id_region) {
        m_id_region = id_region;
        m_est_modifie = true; 
    }
}

void Convoi::set_etat(EtatConvoi etat) {
    if (m_etat != etat) {
        m_etat = etat;
        m_est_modifie = true; 
    }
}

void Convoi::set_horaire_prevue(int minutes) {
    if (m_horaire_prevue != minutes) {
        m_horaire_prevue = minutes;
        m_est_modifie = true; // <--- ICI
    }
}

void Convoi::liberer_voitures(double heure_arrivee) {
    for (auto* v : m_voitures) {
        // (Ces actions activent automatiquement le Dirty Bit des VOITURES)
        if (m_type == TypeConvoi::SORTIE) {
            v->set_etat(EtatVoiture::EN_ROUTE);
            v->set_heure_arrivee(heure_arrivee); 
        } else {
            v->set_etat(EtatVoiture::EN_ATTENTE_GARE); 
        }
    }
    
    // CORRECTION : On ne fait que changer l'état. On garde la taille intacte 
    // pour que le nettoyeur d'agenda puisse faire son calcul !
    if (m_etat != EtatConvoi::TERMINE) {
        m_etat = EtatConvoi::TERMINE;
        m_est_modifie = true; 
    }
}