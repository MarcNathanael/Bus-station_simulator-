#include "PanelInspector.h"
#include "Theme.h"
#include "imgui.h"
#include "Voiture.h"

void PanelInspector::set_convoi(const Convoi* c) {
    m_convoi = c;
    m_is_open = (c != nullptr);
}

void PanelInspector::render() {
    if (!m_is_open) { m_convoi = nullptr; return; }
    ImGui::SetNextWindowPos(ImVec2(460, 200), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(360, 350), ImGuiCond_Appearing);
    
    if (ImGui::Begin("Inspection Convoi", &m_is_open, ImGuiWindowFlags_NoCollapse)) {
        if (m_convoi) {
            ImGui::PushFont(Theme::FontBold);
            ImGui::Text("Convoi #%d", m_convoi->get_id());
            ImGui::PopFont();
            ImGui::Text("Region: %d", m_convoi->get_id_region());
            ImGui::Text("Type: %s", m_convoi->get_type() == TypeConvoi::SORTIE ? "SORTIE" : "ENTREE");
            if (m_convoi->contient_urgence()) ImGui::TextColored(ImVec4(1,0,0,1), "URGENCE !");
            
            ImGui::Separator();
            ImGui::Text("Voitures contenues (%d) :", m_convoi->get_taille());
            if (ImGui::BeginTable("V", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("ID"); ImGui::TableSetupColumn("Coop");
                ImGui::TableSetupColumn("Passagers"); ImGui::TableSetupColumn("Dest");
                ImGui::TableHeadersRow();
                for (const auto* v : m_convoi->get_voitures()) {
                    if (!v) continue;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", v->get_id());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%d", v->get_id_coop());
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d/%d", v->get_passagers(), v->get_places_max());
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%d", v->get_destination());
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}