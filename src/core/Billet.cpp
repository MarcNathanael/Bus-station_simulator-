#include "Billet.h"

// Constructeur
Billet::Billet(int client_id, int voiture_id, int heure_depart_min, int heure_depart_max, double prix)
    : m_client_id(client_id),
      m_voiture_id(voiture_id),
      m_heure_depart_min(heure_depart_min),
      m_heure_depart_max(heure_depart_max),
      m_prix(prix)
{}

// --- Getters ---
int Billet::get_client_id() const { return m_client_id; }
int Billet::get_voiture_id() const { return m_voiture_id; }
int Billet::get_heure_depart_min() const { return m_heure_depart_min; }
int Billet::get_heure_depart_max() const { return m_heure_depart_max; }
double Billet::get_prix() const { return m_prix; }