#include "Convoi.h"

// ==========================================
// Fichier : Convoi.cpp
// ==========================================
#include "Convoi.h"

Convoi::Convoi(int id, TypeConvoi type)
    : m_id(id), 
      m_type(type), 
      m_etat(EtatConvoi::EN_FORMATION), 
      m_horaire_prevue(-1),
      m_contient_urgence(false), // Initialisation explicite à false ici
      m_id_region(0)
{
}
void Convoi::set_contient_urgence(bool valeur) { m_contient_urgence = valeur; }
bool Convoi::contient_urgence() const { return m_contient_urgence; }

int Convoi::get_id_region() const 
{
        return m_id_region;
}
int Convoi::get_id() const { return m_id; }
TypeConvoi Convoi::get_type() const { return m_type; }
EtatConvoi Convoi::get_etat() const { return m_etat; }
int Convoi::get_taille() const { return static_cast<int>(m_voitures.size()); }
int Convoi::get_taille_max() const { return TAILLE_MAX; }
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

bool Convoi::ajouter_voiture(Voiture* v) {
    if (!v) return false;

    // 2. Le convoi doit être en cours de formation et non plein
    if (m_etat != EtatConvoi::EN_FORMATION) return false; // ne peut ajouter qu'en formation
    if (est_plein()) return false;

    // 3. Verification
    if (!voiture_est_disponible(v)) return false;  // ← vérification

    // 4. Validation et transition d'état de la voiture
    v->set_etat(EtatVoiture::EN_CHARGEMENT);

    m_voitures.push_back(v);

    return true;
}

// ─── RETIRER UNE VOITURE (modifié) ────────────────────────
bool Convoi::retirer_voiture(int id_voiture) {
    for (auto it = m_voitures.begin(); it != m_voitures.end(); ++it) {
        if ((*it)->get_id() == id_voiture) {
            // Remet la voiture dans son état d'attente
            if (m_type == TypeConvoi::SORTIE) {
                (*it)->set_etat(EtatVoiture::EN_ATTENTE_GARE);
            } else {
                (*it)->set_etat(EtatVoiture::EN_ATTENTE_STATION);
            }
            m_voitures.erase(it);
            return true;
        }
    }
    return false;
}

bool Convoi::est_plein() const {
    return m_voitures.size() >= TAILLE_MAX;
}

void Convoi::set_id_region(int id_region) noexcept 
{
        m_id_region = id_region;
}

void Convoi::set_etat(EtatConvoi etat) {
    m_etat = etat;
}

void Convoi::set_horaire_prevue(int minutes) {
    m_horaire_prevue = minutes;
}

void Convoi::liberer_voitures(double heure_arrivee) {
    for (auto* v : m_voitures) {
        if (m_type == TypeConvoi::SORTIE) {
            v->set_etat(EtatVoiture::EN_ROUTE);
            v->set_heure_arrivee(heure_arrivee); // Assigne directement l'heure d'arrivée calculée par le simulateur
        } else {
            // Pour une ENTRÉE, le convoi se libère à l'arrivée à la Gare principale
            v->set_etat(EtatVoiture::EN_ATTENTE_GARE); 
            // m_heure_arrivee est automatiquement réinitialisé à -1.0 par Voiture::set_etat
        }
    }
    m_voitures.clear(); // Maintenant on peut vider sereinement, les voitures sont configurées
    m_etat = EtatConvoi::TERMINE;
}