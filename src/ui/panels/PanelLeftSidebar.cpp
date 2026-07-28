#include "PanelLeftSidebar.h"
#include "imgui.h"
#include <iostream>

PanelLeftSidebar::PanelLeftSidebar(Simulateur& simulateur, Planificateur& planificateur, const std::vector<Destination>& destinations)
    : m_simulateur(simulateur)
    , m_planificateur(planificateur)
    , m_destinations(destinations)
{
}

void PanelLeftSidebar::render(bool* p_open) {
    // Définition de la taille et position pour ne pas masquer Madagascar (X commence à 368.6)
    ImGui::SetNextWindowSize(ImVec2(360, 720), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    
    // Fenêtre fixe façon tableau de bord (sans collapse, redimensionnable mais limité)
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    
    if (ImGui::Begin("Controle Simulation", p_open, flags)) {
        
        // --- 1. CONTRÔLE DU TEMPS ---
        if (ImGui::CollapsingHeader("Temps & Vitesse", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button(m_simulateur.est_en_pause() ? "Play" : "Pause", ImVec2(100, 30))) {
                m_simulateur.set_en_pause(!m_simulateur.est_en_pause());
            }
            
            ImGui::SameLine();
            ImGui::TextColored(m_simulateur.est_en_pause() ? ImVec4(1,0,0,1) : ImVec4(0,1,0,1), 
                               m_simulateur.est_en_pause() ? "PAUSE" : "EN COURS");

            ImGui::Text("Vitesse: ");
            ImGui::SameLine();
            if (ImGui::Button("x1")) m_simulateur.set_vitesse(1);
            ImGui::SameLine();
            if (ImGui::Button("x10")) m_simulateur.set_vitesse(10);
            ImGui::SameLine();
            if (ImGui::Button("x100")) m_simulateur.set_vitesse(100);
            ImGui::SameLine();
            if (ImGui::Button("x500")) m_simulateur.set_vitesse(500);
        }

        ImGui::Separator();

        // --- 2. LIVE TUNING ---
        if (ImGui::CollapsingHeader("Parametres Metier (Live)")) {
            ImGui::PushItemWidth(200);
            if (ImGui::SliderInt("Seuil Remplissage (%)", &m_ui_seuil_remplissage, 0, 100)) {
                m_planificateur.set_seuil_remplissage_min(static_cast<double>(m_ui_seuil_remplissage));
            }
            if (ImGui::SliderInt("Taille Max Convoi", &m_ui_taille_max, 1, 20)) {
                m_planificateur.set_taille_max_convoi(m_ui_taille_max);
            }
            if (ImGui::SliderInt("Espacement Min (min)", &m_ui_espacement, 0, 60)) {
                m_planificateur.set_espacement_min(m_ui_espacement);
            }
            if (ImGui::SliderInt("Franchissement (min/voit)", &m_ui_franchissement, 1, 10)) {
                m_planificateur.set_duree_franchissement_voiture(m_ui_franchissement);
            }
            if (ImGui::SliderInt("Delai Achat (min)", &m_ui_delai_achat, 0, 60)) {
                m_planificateur.set_delai_achat_min(m_ui_delai_achat);
            }
            ImGui::PopItemWidth();
        }

        ImGui::Separator();

        // --- 3. BILLETTERIE MANUELLE ---
        if (ImGui::CollapsingHeader("Billetterie Manuelle")) {
            ImGui::Text("Injecter un groupe de passagers :");
            
            const char* preview = m_destinations.empty() ? "" : m_destinations[m_selected_dest_index].get_nom().c_str();
            if (ImGui::BeginCombo("Destination", preview)) {
                for (size_t n = 0; n < m_destinations.size(); n++) {
                    const bool is_selected = (m_selected_dest_index == n);
                    if (ImGui::Selectable(m_destinations[n].get_nom().c_str(), is_selected)) {
                        m_selected_dest_index = n;
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::InputInt("Nb Passagers", &m_nb_passagers);
            m_nb_passagers = (m_nb_passagers < 1) ? 1 : m_nb_passagers; // Sécurité
            
            ImGui::Checkbox("Urgent (VIP)", &m_est_urgent);
            ImGui::Checkbox("Retour (Province -> Gare)", &m_est_retour);

            if (ImGui::Button("Injecter Demande", ImVec2(200, 25))) {
                int id_dest = m_destinations[m_selected_dest_index].get_id();
                m_simulateur.injecter_demande_manuelle(id_dest, m_nb_passagers, m_est_retour, m_est_urgent);
                std::cout << "[UI] Injection manuelle vers " << id_dest << " (" << m_nb_passagers << " passagers)" << std::endl;
            }
        }

        ImGui::Separator();

        // --- 4. PLAGES INTERDITES ---
        if (ImGui::CollapsingHeader("Gestion Travaux")) {
            ImGui::Text("Ajouter une fermeture du portail :");
            ImGui::InputInt("Debut (0-1439)", &m_plage_debut);
            ImGui::InputInt("Fin (0-1439)", &m_plage_fin);
            
            if (m_plage_debut < 0) m_plage_debut = 0;
            if (m_plage_fin > 1439) m_plage_fin = 1439;

            if (ImGui::Button("Ajouter Plage Interdite", ImVec2(200, 25))) {
                if (m_plage_debut != m_plage_fin) {
                    m_simulateur.ajouter_plage_interdite_ui(m_plage_debut, m_plage_fin);
                    std::cout << "[UI] Plage interdite ajoutee: " << m_plage_debut << " a " << m_plage_fin << std::endl;
                } else {
                    std::cerr << "[UI] Erreur: Debut et fin identiques." << std::endl;
                }
            }
        }
    }
    ImGui::End();
}