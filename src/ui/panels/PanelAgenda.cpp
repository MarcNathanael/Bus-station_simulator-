#include "PanelAgenda.h"
#include "Convoi.h"
#include "imgui.h"
#include <string>

PanelAgenda::PanelAgenda(Simulateur& simulateur) : m_simulateur(simulateur) {}

void PanelAgenda::render(bool* p_open) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        if (ImGui::Begin("Agenda", p_open, flags)) {
        
        ImGui::BeginChild("Frise", ImVec2(0, 80), true);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetWindowContentRegionMin();
        ImVec2 p1 = ImGui::GetWindowContentRegionMax();
        ImVec2 origin = ImGui::GetWindowPos();
        
        float x0 = origin.x + p0.x;
        float x1 = origin.x + p1.x;
        float y = origin.y + p0.y + 30.0f;
        
        draw->AddLine(ImVec2(x0, y), ImVec2(x1, y), IM_COL32(100, 100, 100, 255), 2.0f);
        int t_start = m_simulateur.get_temps_continu();
        int t_win = 1440;
        draw->AddLine(ImVec2(x0, y - 15), ImVec2(x0, y + 15), IM_COL32(255, 255, 0, 255), 3.0f);

        auto draw_c = [&](const std::vector<Convoi>& list, bool isS) {
            for (const auto& c : list) {
                if (c.get_etat() == EtatConvoi::PRET) {
                    int h = c.get_horaire_prevue();
                    if (h >= t_start && h <= t_start + t_win) {
                        float x = x0 + ((float)(h - t_start) / t_win) * (x1 - x0);
                        ImU32 col = c.contient_urgence() ? IM_COL32(200, 20, 20, 255) : (isS ? IM_COL32(20, 200, 20, 255) : IM_COL32(200, 130, 20, 255));
                        draw->AddRectFilled(ImVec2(x - 3, y - 20), ImVec2(x + 3, y + 20), col);
                    }
                }
            }
        };
        draw_c(m_simulateur.get_convois_sortie(), true);
        draw_c(m_simulateur.get_convois_entree(), false);
        ImGui::EndChild();

        ImGui::BeginChild("Logs", ImVec2(0, 0), true);
        ImGui::Text("Prochains départs (Gare -> Province):");
        int count = 0;
        for (const auto& c : m_simulateur.get_convois_sortie()) {
            if (c.get_etat() == EtatConvoi::PRET && count < 10) {
                int h = c.get_horaire_prevue();
                int j = h / 1440 + 1;
                int hh = (h % 1440) / 60;
                int mm = h % 60;
                ImGui::TextColored(c.contient_urgence() ? ImVec4(1,0,0,1) : ImVec4(0,1,0,1), 
                                   " J%d %02d:%02d - Dest:%d (%d voit)", j, hh, mm, c.get_id_region(), c.get_taille());
                count++;
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}