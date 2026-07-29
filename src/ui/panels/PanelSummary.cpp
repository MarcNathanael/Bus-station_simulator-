#include "PanelSummary.h"
#include "Theme.h"
#include "imgui.h"

PanelSummary::PanelSummary(Simulateur& simulateur) : m_simulateur(simulateur) {}

void PanelSummary::render(bool* p_open) {
    if (!*p_open) return;
    ImGui::SetNextWindowPos(ImVec2(300, 100), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(680, 520), ImGuiCond_Appearing);
    if (ImGui::Begin("Resumé de la Simulation", p_open, ImGuiWindowFlags_NoCollapse)) {
        ImGui::PushFont(Theme::FontBold);
        ImGui::Text("Bilan Global du Système");
        ImGui::PopFont();
        ImGui::Separator();
        
        ImGui::Text("%s", m_simulateur.get_resume_simulation().c_str());
        
        ImGui::Separator();
        ImGui::Text("Donnees Clients (Base SQL):");
        int std_a = 0, urg_a = 0;
        m_simulateur.get_billetterie().obtenir_compteurs_attente(m_simulateur.get_temps_continu(), std_a, urg_a);
        ImGui::Text("Clients restants en attente Standard: %d", std_a);
        ImGui::Text("Clients restants en attente Urgent: %d", urg_a);
        
        ImGui::Separator();
        if (ImGui::Button("Fermer", ImVec2(-1, 30))) {
            *p_open = false;
        }
    }
    ImGui::End();
}