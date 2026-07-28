#include "PanelRightSidebar.h"
#include "imgui.h"

PanelRightSidebar::PanelRightSidebar(Simulateur& simulateur)
    : m_simulateur(simulateur) {}

void PanelRightSidebar::render(bool* p_open) {
    // Positionnement fixe à droite, sans empiéter sur Madagascar (qui finit vers 933.9)
    ImGui::SetNextWindowSize(ImVec2(320, 720), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(960, 0), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Statistiques Globales", p_open, flags)) {
        
        int std_attente = 0, urg_attente = 0;
        m_simulateur.get_billetterie().obtenir_compteurs_attente(m_simulateur.get_temps_continu(), std_attente, urg_attente);

        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Temps: %d min", m_simulateur.get_temps_continu());
        ImGui::Separator();
        
        ImGui::Text("Etat du Portail:");
        int portail = m_simulateur.get_portail_occupe_jusqua();
        if (portail > m_simulateur.get_temps_continu()) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "OCCUPE jusqu'a %d", portail);
        } else {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "LIBRE");
        }
        
        ImGui::Separator();
        ImGui::Text("File d'attente Gare:");
        ImGui::Text(" - Standards: %d", std_attente);
        ImGui::TextColored(ImVec4(1, 0.3, 0.3, 1), " - Urgents : %d", urg_attente);
    }
    ImGui::End();
}