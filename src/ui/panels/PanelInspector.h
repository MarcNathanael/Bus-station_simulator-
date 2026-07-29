#pragma once
#include "Convoi.h"

class PanelInspector {
public:
    void set_convoi(const Convoi* c);
    void render();
private:
    const Convoi* m_convoi = nullptr;
    bool m_is_open = false;
};