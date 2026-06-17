#ifndef BILLET_H
#define BILLET_H

class Billet {
private:
    int m_client_id;
    int m_voiture_id;
    int m_heure_depart_min;
    int m_heure_depart_max;
    double m_prix;

public:
    // Constructeur unique pour générer un billet valide
    Billet(int client_id, int voiture_id, int heure_depart_min, int heure_depart_max, double prix);

    // Getters indispensables pour DalBillet
    int get_client_id() const;
    int get_voiture_id() const;
    int get_heure_depart_min() const;
    int get_heure_depart_max() const;
    double get_prix() const;
};

#endif // BILLET_H