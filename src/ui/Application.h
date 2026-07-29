#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include "Simulateur.h"
#include "Destination.h"
#include "core/MapRenderer.h"
#include "panels/PanelLeftSidebar.h"
#include "panels/PanelRightSidebar.h"
#include "panels/PanelInspector.h"
#include "panels/PanelAgenda.h"
#include "panels/PanelSummary.h"

class Application {
public:
    Application(Simulateur& simulateur, const std::vector<Destination>& destinations);
    ~Application();
    void run();

private:
    void processEvents();
    void update(sf::Time deltaReal);
    void render();

    sf::RenderWindow m_window;
    Simulateur& m_simulateur;
    MapRenderer m_mapRenderer;
    
    PanelLeftSidebar m_leftPanel;
    PanelRightSidebar m_rightPanel;
    PanelInspector m_inspectorPanel;
    PanelAgenda m_agendaPanel;
    PanelSummary m_summaryPanel;
    
    double m_accumulateur_minutes;
    float m_fraction_visuelle;

    bool m_show_left_panel;
    bool m_show_right_panel;
    bool m_show_agenda_panel;
};
//sf::Clock m_clock;