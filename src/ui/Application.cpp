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
    , m_accumulateur_minutes(0.0)
    , m_fraction_visuelle(0.0f)
    , m_show_left_panel(true)
    , m_show_right_panel(true)
    , m_show_agenda_panel(true)
{
    m_window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(m_window)) {
        std::cerr << "Erreur init ImGui-SFML" << std::endl;
    }
    if (!m_mapRenderer.loadAssets()) {
        std::cerr << "Erreur chargement des assets de la carte" << std::endl;
    }
    Theme::Apply();
}

Application::~Application() {
    ImGui::SFML::Shutdown();
}

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
        
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            m_window.close();
        }
        
        // Gestion du clic souris pour l'inspection des convois
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            // On vérifie que l'UI (ImGui) n'est pas en train d'utiliser la souris
            if (!ImGui::GetIO().WantCaptureMouse) {
                sf::Vector2f mousePos = m_window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                const Convoi* picked = m_mapRenderer.pickConvoi(mousePos, m_simulateur, m_fraction_visuelle);
                m_inspectorPanel.set_convoi(picked);
            }
        }
    }
}

void Application::update(sf::Time deltaReal) {
    if (!m_simulateur.est_en_pause()) {
        double minutes_a_simuler = deltaReal.asSeconds() * m_simulateur.get_vitesse();
        m_accumulateur_minutes += minutes_a_simuler;
        
        while (m_accumulateur_minutes >= 1.0) {
            m_simulateur.avancer_dune_minute();
            m_accumulateur_minutes -= 1.0;
        }
    }
    m_fraction_visuelle = static_cast<float>(m_accumulateur_minutes);
    ImGui::SFML::Update(m_window, deltaReal);
}

void Application::render() {
    m_window.clear(sf::Color(20, 20, 20));

    // 1. Dessin SFML
    m_mapRenderer.draw(m_window, m_simulateur, m_fraction_visuelle);

    // 2. Dessin ImGui
    if (m_show_left_panel) m_leftPanel.render(&m_show_left_panel);
    if (m_show_right_panel) m_rightPanel.render(&m_show_right_panel);
    if (m_show_agenda_panel) m_agendaPanel.render(&m_show_agenda_panel);
    
    m_inspectorPanel.render();

    ImGui::SFML::Render(m_window);
    m_window.display();
}