#pragma once
#include "Simulateur.h"
#include "Planificateur.h"
#include "Destination.h"

class PanelLeftSidebar {
public:
    PanelLeftSidebar(Simulateur& sim, Planificateur& plan, const std::vector<Destination>& dests);
    void render(bool* p_open);
    bool m_show_summary = false;
private:
    Simulateur& m_simulateur;
    Planificateur& m_planificateur;
    const std::vector<Destination>& m_destinations;

    int m_sel_dest = 1;
    int m_nb_pass = 10;
    bool m_urg = false;
    bool m_ret = false;
    int m_debut = 120, m_fin = 180;
    int m_buf_seuil = 50, m_buf_taille = 8, m_buf_espace = 15, m_buf_frac = 2, m_buf_delai = 15;
    int m_flux_mult = 1;
};