#include "PanelAgenda.h"
#include "imgui.h"
#include "Convoi.h"
#include <string>

PanelAgenda::PanelAgenda(Simulateur& simulateur)
    : m_simulateur(simulateur) {}

void PanelAgenda::render(bool* p_open) {
    // Placé en bas, entre le panneau gauche (360) et le droit (960)
    ImGui::SetNextWindowSize(ImVec2(600, 140), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(360, 580), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Agenda Planificateur", p_open, flags)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p_min = ImGui::GetWindowContentRegionMin();
        ImVec2 p_max = ImGui::GetWindowContentRegionMax();
        ImVec2 origin = ImGui::GetWindowPos();
        
        float start_x = origin.x + p_min.x;
        float end_x = origin.x + p_max.x;
        float y = origin.y + (p_min.y + p_max.y) / 2.0f;
        
        // Ligne principale du temps (représente les prochaines 24h)
        draw_list->AddLine(ImVec2(start_x, y), ImVec2(end_x, y), IM_COL32(100, 100, 100, 255), 2.0f);
        
        int time_start = static_cast<int>(m_simulateur.get_temps_continu());
        int time_window = 1440; // 24h
        
        // Marqueurs temporels
        for (int t = time_start; t <= time_start + time_window; t += 360) { // Toutes les 6h
            float x = start_x + ((float)(t - time_start) / time_window) * (end_x - start_x);
            draw_list->AddLine(ImVec2(x, y - 8), ImVec2(x, y + 8), IM_COL32(150, 150, 150, 255), 1.0f);
            std::string label = "J" + std::to_string(t/1440 + 1) + " " + std::to_string((t%1440)/60) + "h";
            draw_list->AddText(ImVec2(x + 5, y - 25), IM_COL32(200, 200, 200, 255), label.c_str());
        }

        // Marqueur du temps présent
        draw_list->AddLine(ImVec2(start_x, y - 15), ImVec2(start_x, y + 15), IM_COL32(255, 255, 0, 255), 3.0f);

        // Affichage des convois
        auto draw_convois = [&](const std::vector<Convoi>& convois, bool isSortie) {
            for (const auto& c : convois) {
                if (c.get_etat() == EtatConvoi::PRET) {
                    int horaire = c.get_horaire_prevue();
                    if (horaire >= time_start && horaire <= time_start + time_window) {
                        float x = start_x + ((float)(horaire - time_start) / time_window) * (end_x - start_x);
                        ImU32 color = c.contient_urgence() ? IM_COL32(200, 20, 20, 255) : 
                                      (isSortie ? IM_COL32(20, 200, 20, 255) : IM_COL32(200, 130, 20, 255));
                        draw_list->AddRectFilled(ImVec2(x - 3, y - 20), ImVec2(x + 3, y + 20), color);
                    }
                }
            }
        };
        
        draw_convois(m_simulateur.get_convois_sortie(), true);
        draw_convois(m_simulateur.get_convois_entree(), false);
    }
    ImGui::End();
}