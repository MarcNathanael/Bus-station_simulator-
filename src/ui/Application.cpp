#include "Application.h"
#include "Theme.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include <iostream>

Application::Application(Simulateur& simulateur, const std::vector<Destination>& destinations)
    : m_window(sf::VideoMode(1280, 720), "Gare Routiere Simulator", sf::Style::Default)
    , m_simulateur(simulateur)
    , m_mapRenderer(destinations)
    , m_leftPanel(simulateur, simulateur.get_planificateur(), destinations)
    , m_rightPanel(simulateur)
    , m_agendaPanel(simulateur)
    , m_summaryPanel(simulateur) // <-- AJOUT CRUCIAL ICI
    , m_accumulateur_minutes(0.0)
    , m_fraction_visuelle(0.0f)
    , m_show_left_panel(true)
    , m_show_right_panel(true)
    , m_show_agenda_panel(true)
{
    m_window.setFramerateLimit(60);
    
    // 1. Init ImGui (Crée le contexte)
    if (!ImGui::SFML::Init(m_window, false)) {
        std::cerr << "Erreur init ImGui-SFML" << std::endl;
    }
    // 2. Charger les polices (Le contexte existe)
    Theme::LoadFonts();
    // 3. Appliquer la texture des polices
    if (!ImGui::SFML::UpdateFontTexture()) {
        std::cerr << "Erreur UpdateFontTexture" << std::endl;
    }
    // 4. Appliquer le style
    Theme::ApplyStyle();

    m_mapRenderer.loadAssets();
}

Application::~Application() { ImGui::SFML::Shutdown(); }

void Application::run() {
    sf::Clock frame_clock;
    while (m_window.isOpen()) {
        processEvents();
        sf::Time delta_real = frame_clock.restart();
        update(delta_real);
        render();
    }
}

void Application::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        ImGui::SFML::ProcessEvent(m_window, event);
        if (event.type == sf::Event::Closed) m_window.close();
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) m_window.close();
        
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (!ImGui::GetIO().WantCaptureMouse) {
                sf::Vector2f mousePos = m_window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                m_inspectorPanel.set_convoi(m_mapRenderer.pickConvoi(mousePos, m_simulateur, m_fraction_visuelle));
            }
        }
    }
}

void Application::update(sf::Time deltaReal) {
    if (!m_simulateur.est_en_pause()) {
        m_accumulateur_minutes += deltaReal.asSeconds() * m_simulateur.get_vitesse();
        while (m_accumulateur_minutes >= 1.0) {
            m_simulateur.avancer_dune_minute();
            m_accumulateur_minutes -= 1.0;
        }
    }
    m_fraction_visuelle = static_cast<float>(m_accumulateur_minutes);
    ImGui::SFML::Update(m_window, deltaReal);
}

void Application::render() {
    m_window.clear(sf::Color(15, 20, 25));
    m_mapRenderer.draw(m_window, m_simulateur, m_fraction_visuelle);

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Affichage")) {
            ImGui::MenuItem("Controles", NULL, &m_show_left_panel);
            ImGui::MenuItem("Statistiques", NULL, &m_show_right_panel);
            ImGui::MenuItem("Agenda", NULL, &m_show_agenda_panel);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    float menuH = ImGui::GetFrameHeight();
    sf::Vector2u win = m_window.getSize();
    float sideW = 300.0f;
    
    if (m_show_left_panel) {
        ImGui::SetNextWindowPos(ImVec2(0, menuH));
        ImGui::SetNextWindowSize(ImVec2(sideW, win.y - menuH));
        m_leftPanel.render(&m_show_left_panel);
    }

    if (m_show_right_panel) {
        ImGui::SetNextWindowPos(ImVec2(win.x - sideW, menuH));
        ImGui::SetNextWindowSize(ImVec2(sideW, (win.y - menuH) * 0.65f));
        m_rightPanel.render(&m_show_right_panel);
    }

    if (m_show_agenda_panel) {
        ImGui::SetNextWindowPos(ImVec2(win.x - sideW, menuH + (win.y - menuH) * 0.65f));
        ImGui::SetNextWindowSize(ImVec2(sideW, (win.y - menuH) * 0.35f));
        m_agendaPanel.render(&m_show_agenda_panel);
    }

    m_inspectorPanel.render();

    // Gestion du panneau de résumé
    if (m_leftPanel.m_show_summary) {
        m_summaryPanel.render(&m_leftPanel.m_show_summary);
    }

    ImGui::SFML::Render(m_window);
    m_window.display();
}