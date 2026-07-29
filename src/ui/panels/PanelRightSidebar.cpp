#include "PanelRightSidebar.h"
#include "Theme.h"
#include "imgui.h"
#include <iomanip>
#include <sstream>

PanelRightSidebar::PanelRightSidebar(Simulateur& simulateur) : m_simulateur(simulateur) {}

void PanelRightSidebar::render(bool* p_open) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("Statistiques", p_open, flags)) {
        int t = m_simulateur.get_temps_continu();
        int j = t / 1440 + 1;
        int h = (t % 1440) / 60;
        int m = t % 60;
        int s = static_cast<int>(60.0f * (m_simulateur.get_temps_continu() - t));

        std::ostringstream oss;
        oss << "J" << j << " " << std::setw(2) << std::setfill('0') << h << ":"
            << std::setw(2) << std::setfill('0') << m << ":"
            << std::setw(2) << std::setfill('0') << s;

        ImGui::PushFont(Theme::FontBold);
        ImGui::Text("%s", oss.str().c_str());
        ImGui::PopFont();
        ImGui::Separator();

        int portail = m_simulateur.get_portail_occupe_jusqua();
        if (portail > m_simulateur.get_temps_continu()) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Portail: OCCUPE");
        } else {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Portail: LIBRE");
        }

        int std_a = 0, urg_a = 0;
        m_simulateur.get_billetterie().obtenir_compteurs_attente(m_simulateur.get_temps_continu(), std_a, urg_a);
        ImGui::Separator();
        ImGui::Text("File d'attente:");
        ImGui::Text(" Standards: %d", std_a);
        ImGui::TextColored(ImVec4(1, 0.3, 0.3, 1), " Urgents : %d", urg_a);

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Plages Interdites")) {
            int idx = 0;
            for (const auto& p : m_simulateur.get_plages_interdites()) {
                int dh = p.get_debut() / 60, dm = p.get_debut() % 60;
                int fh = p.get_fin() / 60, fm = p.get_fin() % 60;
                ImGui::Text(" %02d:%02d -> %02d:%02d", dh, dm, fh, fm);
                ImGui::SameLine(150);
                char buf[20]; snprintf(buf, sizeof(buf), "Suppr##%d", idx);
                if (ImGui::SmallButton(buf)) {
                    m_simulateur.supprimer_plage_interdite_ui(idx);
                    break; 
                }
                idx++;
            }
        }
    }
    ImGui::End();
}