#include "PanelLeftSidebar.h"
#include "Theme.h"
#include "imgui.h"

PanelLeftSidebar::PanelLeftSidebar(Simulateur& sim, Planificateur& plan, const std::vector<Destination>& dests)
    : m_simulateur(sim), m_planificateur(plan), m_destinations(dests) {}

void PanelLeftSidebar::render(bool* p_open) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("Controles", p_open, flags)) {
        
        // --- TEMPS & VITESSE ---
        if (ImGui::CollapsingHeader("Temps & Vitesse", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button(m_simulateur.est_en_pause() ? "Play" : "Pause", ImVec2(-1, 30))) {
                m_simulateur.set_en_pause(!m_simulateur.est_en_pause());
            }
            int vitesses[] = {1, 10, 100, 500};
            for (int v : vitesses) {
                char buf[10]; snprintf(buf, sizeof(buf), "x%d", v);
                bool active = (m_simulateur.get_vitesse() == v);
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
                if (ImGui::Button(buf)) m_simulateur.set_vitesse(v);
                if (active) ImGui::PopStyleColor();
                ImGui::SameLine();
            }
            ImGui::Separator();
            if (ImGui::Button("Terminer Simulation", ImVec2(-1, 30))) {
                m_simulateur.set_en_pause(true);
                m_show_summary = true;
            }
        }

        // --- FLUX TRAFIC ---
        if (ImGui::CollapsingHeader("Flux Trafic")) {
            ImGui::Text("Multiplicateur: x%d", m_flux_mult);
            if (ImGui::Button("Diminuer (-)", ImVec2(120, 25))) {
                m_flux_mult = std::max(1, m_flux_mult - 1);
                m_simulateur.set_multiplicateur_flux(m_flux_mult);
            }
            ImGui::SameLine();
            if (ImGui::Button("Augmenter (+)", ImVec2(120, 25))) {
                m_flux_mult = std::min(20, m_flux_mult + 1);
                m_simulateur.set_multiplicateur_flux(m_flux_mult);
            }
        }

        // --- LIVE TUNING ---
        if (ImGui::CollapsingHeader("Regles Metier")) {
            ImGui::PushItemWidth(150);
            ImGui::SliderInt("Seuil (%)", &m_buf_seuil, 0, 100);
            ImGui::SliderInt("Taille Max", &m_buf_taille, 1, 20);
            ImGui::SliderInt("Espacement", &m_buf_espace, 0, 60);
            ImGui::SliderInt("Franchissement", &m_buf_frac, 1, 10);
            ImGui::SliderInt("Delai Achat", &m_buf_delai, 0, 60);
            ImGui::PopItemWidth();

            if (ImGui::Button("Appliquer", ImVec2(-1, 25))) {
                m_planificateur.set_seuil_remplissage_min(m_buf_seuil);
                m_planificateur.set_taille_max_convoi(m_buf_taille);
                m_planificateur.set_espacement_min(m_buf_espace);
                m_planificateur.set_duree_franchissement_voiture(m_buf_frac);
                m_planificateur.set_delai_achat_min(m_buf_delai);
            }
        }

        // --- INJECTION ---
        if (ImGui::CollapsingHeader("Billetterie Manuelle")) {
            std::string prev = m_destinations.empty() ? "" : m_destinations[m_sel_dest].get_nom();
            if (ImGui::BeginCombo("Destination", prev.c_str())) {
                for (size_t n = 0; n < m_destinations.size(); n++) {
                    bool is_sel = (m_sel_dest == n);
                    if (ImGui::Selectable(m_destinations[n].get_nom().c_str(), is_sel)) m_sel_dest = n;
                    if (is_sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::InputInt("Passagers", &m_nb_pass);
            if (m_nb_pass < 1) m_nb_pass = 1;
            ImGui::Checkbox("Urgent", &m_urg);
            ImGui::Checkbox("Retour", &m_ret);
            if (ImGui::Button("Injecter", ImVec2(-1, 25))) {
                m_simulateur.injecter_demande_manuelle(m_destinations[m_sel_dest].get_id(), m_nb_pass, m_ret, m_urg);
            }
        }

        // --- TRAVAUX ---
        if (ImGui::CollapsingHeader("Gestion Travaux")) {
            ImGui::InputInt("Debut", &m_debut);
            ImGui::InputInt("Fin", &m_fin);
            if (ImGui::Button("Ajouter Plage", ImVec2(-1, 25))) {
                if (m_debut != m_fin) m_simulateur.ajouter_plage_interdite_ui(m_debut, m_fin);
            }
        }
    }
    ImGui::End();
}