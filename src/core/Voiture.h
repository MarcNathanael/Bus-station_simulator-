#pragma once
#include <string>
#include <stdexcept>

enum class EtatVoiture {
    EN_ATTENTE_GARE,
    EN_CHARGEMENT,
    EN_ROUTE,
    EN_ATTENTE_STATION,
    EN_ROUTE_RETOUR
};

// Fonction utilitaire pour parser l'état depuis un CSV
// inline exige que le coprs de la fonction soit visible dans le hearders qui l'inclut , pas de prototype
inline EtatVoiture stringToEtatVoiture(const std::string& s)
{
    if (s == "EN_ATTENTE_GARE") return EtatVoiture::EN_ATTENTE_GARE;
    if (s == "EN_CHARGEMENT") return EtatVoiture::EN_CHARGEMENT;
    if (s == "EN_ROUTE") return EtatVoiture::EN_ROUTE;
    if (s == "EN_ATTENTE_STATION") return EtatVoiture::EN_ATTENTE_STATION;
    if (s == "EN_ROUTE_RETOUR") return EtatVoiture::EN_ROUTE_RETOUR;
    throw std::runtime_error("EtatVoiture inconnu : " + s);
}

class Voiture {
    public:
        // Constructeur simple (utilisé par défaut dans les tests)
        Voiture(int id, int places_max, int destination_initiale, int t_charge, int t_decharge);
        // Constructeur complet (utilisé par Configuration)
        Voiture(int id, int id_coop, int id_destination, int id_position,
            int capacite_max, int places_libres, EtatVoiture etat, int horaire_depart,
            int t_charge, int t_decharge);

        // Getters
        int get_id() const;
        int get_id_coop() const;
        int get_places_libres() const;
        int get_places_max() const;
        int get_horaire_depart() const;
        EtatVoiture get_etat() const;
        int get_position() const;
        int get_destination() const;
        double get_heure_arrivee() const;
        int get_passagers() const;

        // Actions métier
        bool embarquer(int nb_passagers);
        bool est_pleine() const;
        bool debarquer(int nb);
        bool is_dirty() const;
        void clear_dirty();


        // Setters utiliser par la cooperative 
        void set_etat(EtatVoiture etat);
        void set_position(int pos);
        void set_horaire_depart(int minutes);
        void set_heure_arrivee(double heure);
        void set_destination(int id_dest);
        void debarquer_tous(); // Pour vider la voiture à l'arrivée immédiate

        int get_temps_chargement() const { return m_temps_chargement; }
    int get_temps_dechargement() const { return m_temps_dechargement; }
    private:
    // cooperative peur directemment mofidier les attributs priver de voiture 
        friend class Cooperative;

        int m_id;
        int m_id_coop;
        int m_nb_places_libres;
        int m_nb_places_max;
        int m_horaire_depart;
        EtatVoiture m_etat;
        int m_id_position;
        int m_id_destination;
        double m_heure_arrivee; // Heure absolue à laquelle la voiture termine son transit
        Voiture* m_suivant;
        bool m_est_modifie; // Le Dirty Bit
        // AJOUT : Paramètres de temps d'opération injectés
        int m_temps_chargement;
        int m_temps_dechargement;
};