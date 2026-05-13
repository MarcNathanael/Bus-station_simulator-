#pragma once

class PlageInterdite {
public:
    PlageInterdite(int debut, int fin);
    int get_debut() const;
    int get_fin() const;
private:
    int m_debut; // minutes depuis minuit
    int m_fin;
};