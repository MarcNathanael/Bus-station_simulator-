#pragma once
#include "Simulateur.h"
#include "Planificateur.h"
#include "Destination.h"
#include <vector>
#include <string>

class PanelLeftSidebar {
public:
    PanelLeftSidebar(Simulateur& simulateur, Planificateur& planificateur, const std::vector<Destination>& destinations);
    void render(bool* p_open);

private:
    Simulateur& m_simulateur;
    Planificateur& m_planificateur;
    const std::vector<Destination>& m_destinations;

    // Variables pour les formulaires d'injection
    int m_selected_dest_index = 0;
    int m_nb_passagers = 10;
    bool m_est_urgent = false;
    bool m_est_retour = false;
    
    // Variables pour les plages interdites
    int m_plage_debut = 120;
    int m_plage_fin = 180;

    // Variables pour le Live Tuning (initialisées avec les valeurs par défaut du métier)
    int m_ui_seuil_remplissage = 50;
    int m_ui_taille_max = 8;
    int m_ui_espacement = 15;
    int m_ui_franchissement = 2;
    int m_ui_delai_achat = 15;
};