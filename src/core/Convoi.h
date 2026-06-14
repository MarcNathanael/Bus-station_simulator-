// Convoi.h
#pragma once
#include <vector>
#include <stdexcept>
#include "Voiture.h" 

enum class TypeConvoi {
    SORTIE,
    ENTREE
};

enum class EtatConvoi {
    EN_FORMATION,      // en attente de remplissage
    PRET,              // complet ou fermé, en attente de passage
    EN_FRANCHISSEMENT, // en train de passer le portail
    TERMINE,           // passé, voitures libérées
    EN_TRANSIT
};

class Voiture; // déclaration anticipée

class Convoi {
    public:
        Convoi(int id, TypeConvoi type);

        // Getters
        int get_id() const;
        TypeConvoi get_type() const;
        EtatConvoi get_etat() const;
        int get_taille() const;
        int get_taille_max() const;
        int get_horaire_prevue() const; // minutes depuis minuit
        const std::vector<Voiture*>& get_voitures() const;
        int get_id_region() const;
        bool contient_urgence() const;
        
        // Gestion des voitures
        bool ajouter_voiture(Voiture* v);
        bool retirer_voiture(int id_voiture);
        bool est_plein() const;
        
        // Changement d'état
        void set_id_region(int id_region) noexcept;
        void set_contient_urgence(bool valeur);
        void set_etat(EtatConvoi etat);
        void set_horaire_prevue(int minutes);

        // Libère toutes les voitures (les retire du convoi)
        void liberer_voitures(double heure_arrivee = -1.0);

        bool is_dirty() const;
        void clear_dirty();
    private:
        bool voiture_est_disponible(const Voiture* v) const;

        int m_id;
        TypeConvoi m_type;
        EtatConvoi m_etat;
        int m_horaire_prevue;
        std::vector<Voiture*> m_voitures; // evite la copie
        static const int TAILLE_MAX = 8;
        bool m_contient_urgence = false;
        int m_id_region; // Ajout : Stocke l'ID de la province associée au convoi
        bool m_est_modifie = false; // Le Dirty Bit
};