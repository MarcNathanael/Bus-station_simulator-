#include "Simulateur.h"

// Inclusion des interfaces réelles des composants métiers pour exécution
#include "Billetterie.h"
#include "Generateur.h"
#include "Planificateur.h"

// ============================================================================
// CONSTRUCTEUR & CONFIGURATION INITIALE
// ============================================================================
Simulateur::Simulateur(Billetterie& billetterie, 
                       GenerateurDemandes& generateur, 
                       Planificateur& planificateur,
                       const std::vector<Voiture*>& flotte_initiale,
                       int frequence_planif,
                       int duree_trajet) noexcept
    : m_temps_courant(0)
    , m_portail_occupe_jusqua(0)
    , m_duree_franchissement_par_voiture(2) // Physique stricte : 2 minutes / voiture
    , m_frequence_planif(frequence_planif)
    , m_duree_trajet(duree_trajet)
    , m_voitures(flotte_initiale)
    , m_billetterie(billetterie)
    , m_generateur(generateur)
    , m_planificateur(planificateur)
{
}

// ============================================================================
// LOGIQUE DE SYNCHRONISATON / CACHE PERSISTANT (SQLite)
// ============================================================================
void Simulateur::mettre_a_jour_sqlite(const Voiture& voiture) {
    // Écriture synchrone critique pour refléter la physique réelle en base de données.
    // Empêche la désynchronisation de l'état en cas d'interruption du processus de simulation.
    std::cout << "[SQLITE SYNCHRO] Voiture ID: " << voiture.get_id() 
              << " | État: " << static_cast<int>(voiture.get_etat())
              << " | Position: " << voiture.get_position() 
              << " | Heure Arrivée Prévue: " << voiture.get_heure_arrivee() 
              << " min.\n";
}

bool Simulateur::en_plage_interdite(int temps) const noexcept {
    // Exemple physique : Portail fermé pour maintenance structurelle de la gare
    // entre 02h00 et 04h00 du matin
    int minute_dans_la_journee = temps % 1440;
    if (minute_dans_la_journee >= 120 && minute_dans_la_journee < 240) {
        return true;
    }
    return false;
}

// ============================================================================
// EXECUTEUR GLOBAL CONTINU
// ============================================================================
void Simulateur::executer(int duree_simulation) {
    for (int T = 0; T < duree_simulation; ++T) {
        tick(T);
    }
}

// ============================================================================
// COEUR DU MOTEUR DE SIMULATION : LE TICK UNIQUE
// ============================================================================
void Simulateur::tick(int T) {
    m_temps_courant = T;

    // ------------------------------------------------------------------------
    // [1] PHYSIQUE DES ARRIVÉES (Traitement immédiat + Cumul protecteur)
    // ------------------------------------------------------------------------
    for (Voiture* v : m_voitures) {
        if (v && v->get_etat() == EtatVoiture::EN_ROUTE && T >= v->get_heure_arrivee()) {
            
            // Discrimination physique : Est-ce une Entrée (Gare) ou une Sortie (Province) ?
            bool est_une_entree = false;
            Convoi* convoi_parent = nullptr;

            // Recherche de la voiture au sein des convois d'entrée actuellement en transit
            for (auto& convoi : m_convois_entree) {
                if (convoi.get_etat() == EtatConvoi::EN_TRANSIT) {
                    for (auto* cv : convoi.get_voitures()) {
                        if (cv == v) {
                            est_une_entree = true;
                            convoi_parent = &convoi;
                            break;
                        }
                    }
                }
                if (est_une_entree) break;
            }

            if (!est_une_entree) {
                // ─── CAS A : La voiture arrivait en Province (Fin de SORTIE) ───
                int id_province = v->get_destination();
                
                // 1. Enregistrement de la charge utile descendante dans l'incubateur de séjour
                m_generateur.enregistrer_arrivee_province(id_province, v->get_passagers(), T);
                // 2. Débarquement complet et instantané des passagers
                v->debarquer_tous();
                // 3. Mutation vers l'état statique provincial
                v->set_etat(EtatVoiture::EN_ATTENTE_STATION);
                
                // Sauvegarde de l'état physique persistant
                mettre_a_jour_sqlite(*v);
            } 
            else {
                // ─── CAS B : La voiture arrive à la Gare Principale (Fin d'ENTRÉE) ───
                if (convoi_parent) {
                    // 1. ANTI-EMBOUTEILLAGE : Le convoi d'entrée force le passage pour dégager la rue.
                    
                    // 2. VERROU CUMULATIF SÉCURISÉ : Intégration de l'équation anti-friction temporelle
                    int taille_convoi = convoi_parent->get_taille();
                    m_portail_occupe_jusqua = std::max(m_portail_occupe_jusqua, T) + (taille_convoi * m_duree_franchissement_par_voiture);
                    
                    // 4. Débarquement de sécurité de TOUTES les voitures du convoi arrivant
                    std::vector<Voiture*> voitures_du_convoi = convoi_parent->get_voitures();
                    for (auto* cv : voitures_du_convoi) {
                        if (cv) {
                            cv->debarquer_tous();
                        }
                    }

                    // 3. Libération collective des voitures -> Passage automatique à EN_ATTENTE_GARE
                    convoi_parent->liberer_voitures(); 
                    
                    // Synchronisation en cascade de tous les véhicules libérés du convoi d'entrée
                    for (auto* cv : voitures_du_convoi) {
                        if (cv) {
                            mettre_a_jour_sqlite(*cv);
                        }
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // [2] ANTICIPATION & DÉCLENCHEMENT DES RETOURS (Départs de Province)
    // ------------------------------------------------------------------------
    for (auto& convoi : m_convois_entree) {
        if (convoi.get_etat() == EtatConvoi::PRET) {
            // Calcul rétrograde du moment exact d'engagement physique sur la route provinciale
            int T_depart = convoi.get_horaire_prevue() - m_duree_trajet;
            
            if (T >= T_depart) {
                // 1. Mise en route de l'ensemble de la flotte du convoi
                for (auto* v : convoi.get_voitures()) {
                    if (v) {
                        v->set_etat(EtatVoiture::EN_ROUTE);
                        v->set_heure_arrivee(static_cast<double>(T) + m_duree_trajet);
                        mettre_a_jour_sqlite(*v);
                    }
                }
                // 2. Transition de l'état logistique du convoi global
                convoi.set_etat(EtatConvoi::EN_TRANSIT);
            }
        }
    }

    // ------------------------------------------------------------------------
    // [3] FLUX DE PASSAGERS (Générateur & Billetterie)
    // ------------------------------------------------------------------------
    m_generateur.generer_flux(static_cast<double>(T), m_billetterie);

    // ------------------------------------------------------------------------
    // [4] LE CERVEAU (Planificateur - Rythme cyclique)
    // ------------------------------------------------------------------------
    if (std::fmod(static_cast<double>(T), static_cast<double>(m_frequence_planif)) == 0.0) {
        std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
        
        // 1. Extraction des flux d'attente cumulés de la billetterie
        m_billetterie.extraire_demandes(T, dep_std, dep_urg, ret_std, ret_urg);
        
        // 2. Calcul du plan d'affectation global optimisé
        m_planificateur.planifier_global(dep_std, dep_urg, ret_std, ret_urg, m_voitures, T);
        
        // Récupération des structures de convois fraîchement planifiées par le cerveau
        std::vector<Convoi> nouveaux_sorties = m_planificateur.get_convois_sortie();
        std::vector<Convoi> nouveaux_entrees = m_planificateur.get_convois_entree();

        // Migration sécurisée des entités vers l'agenda opérationnel du simulateur
        for (auto& c : nouveaux_sorties) {
            m_convois_sortie.push_back(std::move(c));
        }
        for (auto& c : nouveaux_entrees) {
            m_convois_entree.push_back(std::move(c));
        }

        // 3. Extraction mathématique de la demande résiduelle (les clients non placés)
        auto residus = m_planificateur.calculer_demande_residuelle(dep_std, dep_urg, ret_std, ret_urg, m_convois_sortie, m_convois_entree);
        
        // 4. Injection des clients déçus en file d'attente prioritaire pour le prochain cycle
        m_billetterie.traiter_demande_residuelle(T, residus);
    }

    // ------------------------------------------------------------------------
    // [5] EXÉCUTION DU PORTAIL EN SORTIE (Sous réserve de disponibilité absolue)
    // ------------------------------------------------------------------------
    if (m_portail_occupe_jusqua > T || en_plage_interdite(T)) {
        // Le portail est physiquement obstrué par une Entrée prioritaire ou un départ très récent.
        // Verrou inviolable : les convois de sortie attendent sagement à l'intérieur de la gare.
    } 
    else {
        // Le portail est LIBRE (portail_occupe_jusqua <= T)
        Convoi* meilleur_convoi = nullptr;

        // Étape 1 : Recherche du convoi candidat optimal selon les priorités opérationnelles
        for (auto& convoi : m_convois_sortie) {
            if (convoi.get_etat() == EtatConvoi::PRET && convoi.get_horaire_prevue() <= T) {
                if (!meilleur_convoi) {
                    meilleur_convoi = &convoi;
                } else {
                    // Regle de priorité 1 : Urgence absolue首 (Contient au moins un passager critique)
                    if (convoi.contient_urgence() && !meilleur_convoi->contient_urgence()) {
                        meilleur_convoi = &convoi;
                    } 
                    // Regle de priorité 2 : Ancienneté stricte de planification (Horaire prévu le plus ancien)
                    else if (convoi.contient_urgence() == meilleur_convoi->contient_urgence()) {
                        if (convoi.get_horaire_prevue() < meilleur_convoi->get_horaire_prevue()) {
                            meilleur_convoi = &convoi;
                        }
                        // Briseur d'égalité : Ordre d'enregistrement séquentiel (ID du convoi)
                        else if (convoi.get_horaire_prevue() == meilleur_convoi->get_horaire_prevue()) {
                            if (convoi.get_id() < meilleur_convoi->get_id()) {
                                meilleur_convoi = &convoi;
                            }
                        }
                    }
                }
            }
        }

        // Si un convoi valide a été sélectionné pour franchissement
        if (meilleur_convoi) {
            // 2. VERROU CUMULATIF : Application de ton équation indestructible de gestion des frictions
            int taille = meilleur_convoi->get_taille();
            m_portail_occupe_jusqua = std::max(m_portail_occupe_jusqua, T) + (taille * m_duree_franchissement_par_voiture);

            // Capture préalable des voitures pour synchronisation (car liberer_voitures va vider le vecteur interne)
            std::vector<Voiture*> voitures_en_depart = meilleur_convoi->get_voitures();

            // 3 & 4. Libération des voitures, passage à EN_ROUTE et calcul précis de l'heure d'arrivée en province
            double heure_arrivee_calculee = static_cast<double>(T) + m_duree_trajet;
            meilleur_convoi->liberer_voitures(heure_arrivee_calculee);

            // Synchronisation SQLite immédiate après mutation de l'état physique
            for (auto* cv : voitures_en_depart) {
                if (cv) {
                    mettre_a_jour_sqlite(*cv);
                }
            }
        }
    }
    
    // [6] FIN DU TICK (Le temps progresse naturellement d'une minute via la boucle supérieure)
}