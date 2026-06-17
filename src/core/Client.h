#ifndef CLIENT_H
#define CLIENT_H

class Client {
private:
    int m_id;
    int m_destination_id;
    int m_t_min;
    int m_t_max;
    bool m_priorite; // true si urgent, false si standard

public:
    // Constructeur complet (utilisé par DalClient::charger_tout)
    Client(int id, int destination_id, int t_min, int t_max, bool priorite);
    
    // Constructeur sans ID (utilisé lors de la création avant insertion DB - l'ID sera généré par SQLite)
    Client(int destination_id, int t_min, int t_max, bool priorite);

    // Getters indispensables pour DalClient
    int get_id() const;
    int get_destination_id() const;
    int get_t_min() const;
    int get_t_max() const;
    bool est_urgent() const;

    // Setters utiles pour l'évolution dans le simulateur
    void set_priorite(bool priorite);
    void set_id(int id);
};

#endif // CLIENT_H