#pragma once

class PlageInterdite {
public:
    PlageInterdite(int debut, int fin);

    int get_debut() const;
    int get_fin() const;
    int get_duree() const;                    // durée en minutes

    bool contient(int horaire) const;          // l'horaire est-il dans la plage ?
    bool est_trop_proche(int horaire, int marge) const; // l'horaire est-il à moins de 'marge' minutes ?

private:
    int m_debut;  // minutes depuis minuit
    int m_fin;    // minutes depuis minuit
};