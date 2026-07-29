#pragma once
#include "Simulateur.h"

class PanelRightSidebar {
public:
    PanelRightSidebar(Simulateur& simulateur);
    void render(bool* p_open);
private:
    Simulateur& m_simulateur;
};