#include "Simulateur.h"

// ============================================================================
// CONSTRUCTEUR
// ============================================================================
Simulateur::Simulateur(std::vector<Voiture*>& flotte_globale,
                       const std::vector<PlageInterdite>& plages,
                       const std::unordered_map<int, int>& durees_trajet,
                       Billetterie& billetterie,
                       GenerateurDemandes& generateur,
                       Planificateur& planificateur,
                       int frequence_planif,
                       int duree_franchissement)
    : m_temps_courant(0)
    , m_portail_occupe_jusqua(0)
    , m_duree_franchissement_par_voiture(duree_franchissement)
    , m_frequence_planif(frequence_planif)
    , m_voitures_flotte(flotte_globale)
    , m_plages_interdites(plages)
    , m_durees_trajet(durees_trajet)
    , m_billetterie(billetterie)
    , m_generateur(generateur)
    , m_planificateur(planificateur)
{
}

// ============================================================================
// PERSISTANCE & VÉRIFICATIONS PHYSIQUES
// ============================================================================
void Simulateur::mettre_a_jour_sqlite(const Voiture& voiture) const {
    // Écriture asynchrone (mockée ici) pour la source de vérité.
    std::cout << "[SQLITE] MAJ Voiture " << voiture.get_id() 
              << " | Etat: " << static_cast<int>(voiture.get_etat()) 
              << " | Arrivee: " << voiture.get_heure_arrivee() << "\n";
}

bool Simulateur::en_plage_interdite(int temps) const noexcept {
    int heure_circulaire = temps % 1440;
    for (const auto& plage : m_plages_interdites) {
        int debut = plage.get_debut();
        int fin = plage.get_fin();
        
        if (debut < fin) {
            if (heure_circulaire >= debut && heure_circulaire < fin) return true;
        } else {
            // Plage chevauchant minuit
            if (heure_circulaire >= debut || heure_circulaire < fin) return true;
        }
    }
    return false;
}

// ============================================================================
// BOUCLE PRINCIPALE
// ============================================================================
void Simulateur::executer(int duree_simulation) {
    for (int T = 0; T < duree_simulation; ++T) {
        tick(T);
    }
}

// ============================================================================
// LOGIQUE TEMPORELLE MÉTIER (LE TICK)
// ============================================================================
void Simulateur::tick(int T) {
    m_temps_courant = T;

    // L'architecture repose sur le fait que le Planificateur fournit un accès modifiable
    // à ses listes pour permettre au Simulateur de mettre à jour l'état physique des convois.
    std::vector<Convoi>& convois_entree = m_planificateur.get_convois_entree();
    std::vector<Convoi>& convois_sortie = m_planificateur.get_convois_sortie();

    // ------------------------------------------------------------------------
    // [1] DÉPARTS DE PROVINCE (Anticipation des retours vers la capitale)
    // ------------------------------------------------------------------------
    for (auto& convoi : convois_entree) {
        if (convoi.get_etat() == EtatConvoi::PRET && convoi.get_taille() > 0) {
            
            // ÉTAPE 2 : On lit la région depuis le convoi, et non plus depuis la voiture
            int id_prov = convoi.get_id_region(); 
            int duree_trajet = m_durees_trajet.at(id_prov);
            
            // Calcul de l'heure exacte où le convoi doit démarrer pour arriver à l'heure prévue au portail
            int t_depart = convoi.get_horaire_prevue() - duree_trajet;
            
            if (T >= t_depart) {
                for (auto* v : convoi.get_voitures()) {
                    if (v) {
                        v->set_etat(EtatVoiture::EN_ROUTE);
                        
                        // ÉTAPE 3 : On force la destination vers la capitale (ID 0)
                        v->set_destination(0); 
                        
                        v->set_heure_arrivee(static_cast<double>(convoi.get_horaire_prevue()));
                        mettre_a_jour_sqlite(*v);
                    }
                }
                convoi.set_etat(EtatConvoi::EN_TRANSIT);
            }
        }
    }

    // ------------------------------------------------------------------------
    // [2] ARRIVÉES EN PROVINCE (Déclenchement des Incubateurs / Sorties terminées)
    // ------------------------------------------------------------------------
    for (auto* v : m_voitures_flotte) {
        if (v && v->get_etat() == EtatVoiture::EN_ROUTE && v->get_destination() != 0) { // 0 = Capitale
            if (T >= v->get_heure_arrivee()) {
                int id_province = v->get_destination();
                
                // Injection des passagers dans le séjour de la province
                m_generateur.enregistrer_arrivee_province(id_province, v->get_passagers(), static_cast<double>(T));
                
                v->debarquer_tous();
                v->set_etat(EtatVoiture::EN_ATTENTE_STATION);
                mettre_a_jour_sqlite(*v);
            }
        }
    }

    // ------------------------------------------------------------------------
    // [3] ARRIVÉES À LA GARE PRINCIPALE (Le Portail des Entrées - Anti-Embouteillage)
    // ------------------------------------------------------------------------
    for (auto& convoi : convois_entree) {
        if (convoi.get_etat() == EtatConvoi::EN_TRANSIT) {
            if (T >= convoi.get_horaire_prevue()) {
                
                // 1 & 2. Force le passage et verrouille le portail en cumulant le temps
                int duree_franchissement = convoi.get_taille() * m_duree_franchissement_par_voiture;
                m_portail_occupe_jusqua = std::max(m_portail_occupe_jusqua, T) + duree_franchissement;

                // 3. Sauvegarde car la libération vide la liste interne du convoi
                std::vector<Voiture*> voitures_arrivantes = convoi.get_voitures();

                // 4. Libère le convoi (passe les voitures EN_ATTENTE_GARE et le convoi TERMINE)
                convoi.liberer_voitures(-1.0);

                // 5. Vide les passagers (fin du voyage) et synchronise
                for (auto* v : voitures_arrivantes) {
                    if (v) {
                        v->debarquer_tous();
                        mettre_a_jour_sqlite(*v);
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // [4] FLUX DE PASSAGERS (Générateur & Billetterie)
    // ------------------------------------------------------------------------
    m_generateur.generer_flux(static_cast<double>(T), m_billetterie);

    // ------------------------------------------------------------------------
    // [5] LE CERVEAU (Activation Cyclique du Planificateur)
    // ------------------------------------------------------------------------
if (T % m_frequence_planif == 0)
{        
        std::vector<Voiture*> voitures_gare;
        std::unordered_map<int, std::vector<Voiture*>> map_provinces;

        // Préparation du snapshot de la flotte pour le Planificateur
        for (auto* v : m_voitures_flotte) {
            if (v) {
                if (v->get_etat() == EtatVoiture::EN_ATTENTE_GARE) {
                    voitures_gare.push_back(v);
                } 
                else if (v->get_etat() == EtatVoiture::EN_ATTENTE_STATION) {
                    map_provinces[v->get_destination()].push_back(v);
                }
            }
        }

        std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
        m_billetterie.extraire_demandes(static_cast<double>(T), dep_std, dep_urg, ret_std, ret_urg);

        // Planification (inclut l'auto-nettoyage des convois TERMINE)
        m_planificateur.planifier_global(dep_std, dep_urg, ret_std, ret_urg, voitures_gare, map_provinces, static_cast<double>(T));

        // Calcul et rapatriement des résidus vers la billetterie
        auto residus = m_planificateur.calculer_demande_residuelle(dep_std, dep_urg, ret_std, ret_urg, convois_sortie, convois_entree);
        
        m_billetterie.traiter_demande_residuelle(static_cast<double>(T), residus.first, residus.second);
    }

    // ------------------------------------------------------------------------
    // [6] DÉPARTS DE LA GARE PRINCIPALE (Le Portail des Sorties)
    // ------------------------------------------------------------------------
    if (m_portail_occupe_jusqua > T || en_plage_interdite(T)) {
        // Le portail est physiquement obstrué ou interdit, on ne fait rien.
    } 
    else {
        Convoi* meilleur_candidat = nullptr;

        // Recherche du meilleur convoi de SORTIE prêt à partir
        for (auto& convoi : convois_sortie) {
            if (convoi.get_etat() == EtatConvoi::PRET && convoi.get_horaire_prevue() <= T) {
                if (!meilleur_candidat) {
                    meilleur_candidat = &convoi;
                } else {
                    // Priorité 1 : Urgence
                    if (convoi.contient_urgence() && !meilleur_candidat->contient_urgence()) {
                        meilleur_candidat = &convoi;
                    } 
                    // Priorité 2 : Ancienneté d'horaire prévu
                    else if (convoi.contient_urgence() == meilleur_candidat->contient_urgence()) {
                        if (convoi.get_horaire_prevue() < meilleur_candidat->get_horaire_prevue()) {
                            meilleur_candidat = &convoi;
                        }
                    }
                }
            }
        }

        if (meilleur_candidat && meilleur_candidat->get_taille() > 0) {
            // 2. VERROU CUMULATIF
            int duree_franchissement = meilleur_candidat->get_taille() * m_duree_franchissement_par_voiture;
            m_portail_occupe_jusqua = std::max(m_portail_occupe_jusqua, T) + duree_franchissement;

            // Détermination de la durée du trajet
            int id_dest = meilleur_candidat->get_voitures().front()->get_destination();
            int duree_trajet = m_durees_trajet.at(id_dest);
            double heure_arrivee_province = static_cast<double>(T + duree_trajet);

            // Sauvegarde avant libération destructrice
            std::vector<Voiture*> voitures_partantes = meilleur_candidat->get_voitures();

            // 3. Libération du convoi (Passe EN_ROUTE et configure l'arrivée)
            meilleur_candidat->liberer_voitures(heure_arrivee_province);

            // Synchronisation
            for (auto* v : voitures_partantes) {
                if (v) mettre_a_jour_sqlite(*v);
            }
        }
    }

    // [7] FIN DU TICK
}