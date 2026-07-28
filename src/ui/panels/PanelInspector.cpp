#include "PanelInspector.h"
#include "imgui.h"
#include "Voiture.h"

void PanelInspector::set_convoi(const Convoi* c) {
    m_convoi = c;
    m_is_open = (c != nullptr); // Ouvre la fenêtre si un convoi est sélectionné
}

void PanelInspector::render() {
    if (!m_is_open) {
        m_convoi = nullptr;
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(460, 200), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(360, 300), ImGuiCond_Appearing);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    // On passe &m_is_open pour activer la croix de fermeture (X) native
    if (ImGui::Begin("Inspection Convoi", &m_is_open, flags)) {
        
        if (m_convoi) {
            ImGui::Text("ID Convoi : %d", m_convoi->get_id());
            ImGui::Text("Type      : %s", m_convoi->get_type() == TypeConvoi::SORTIE ? "SORTIE (Gare -> Prov)" : "ENTREE (Prov -> Gare)");
            ImGui::Text("Region    : %d", m_convoi->get_id_region());
            ImGui::Text("Horaire   : %d min", m_convoi->get_horaire_prevue());
            
            if (m_convoi->contient_urgence()) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "URGENCE MEDICALE A BORD !");
            }
            
            ImGui::Separator();
            ImGui::Text("Voitures contenues (%d) :", m_convoi->get_taille());
            
            if (ImGui::BeginTable("VoituresTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("ID");
                ImGui::TableSetupColumn("Coop");
                ImGui::TableSetupColumn("Etat");
                ImGui::TableSetupColumn("Passagers");
                ImGui::TableSetupColumn("Destination");
                ImGui::TableHeadersRow();

                for (const auto* v : m_convoi->get_voitures()) {
                    if (!v) continue;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", v->get_id());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%d", v->get_id_coop());
                    ImGui::TableSetColumnIndex(2); ImGui::Text("EN_ROUTE");
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%d/%d", v->get_passagers(), v->get_places_max());
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%d", v->get_destination());
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}