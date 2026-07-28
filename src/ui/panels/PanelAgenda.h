#pragma once
#include "Simulateur.h"

class PanelAgenda {
public:
    PanelAgenda(Simulateur& simulateur);
    void render(bool* p_open);

private:
    Simulateur& m_simulateur;
};