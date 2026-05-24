#include "Planificateur.h"
#include <algorithm>
#include <cmath>

// ────────────────────────────────────────────────────────────────
// CONSTRUCTEUR
// ────────────────────────────────────────────────────────────────
Planificateur::Planificateur(const std::unordered_map<int, Destination>& destinations,
                             const std::unordered_map<int, Cooperative>& cooperatives,
                             const std::vector<PlageInterdite>& plages,
                             const std::unordered_map<std::string, int>& parametres)
    : m_destinations(destinations)
    , m_cooperatives(cooperatives)
    , m_plages(plages)
    , m_parametres(parametres)
{
    // Chargement des paramètres avec des valeurs par défaut si absents
    m_espacement_min             = lire_parametre("espacement_min_entre_occupation_convois", 15);
    m_franchissement_par_voiture = lire_parametre("duree_franchissement_voiture", 2);
    m_delai_achat_min            = lire_parametre("duree_min_achat_avant_depart", 15);
    m_debut_journee              = lire_parametre("debut_journee", 0);
    m_fin_journee                = lire_parametre("fin_journee", 1440);
    m_taille_max_convoi          = lire_parametre("taille_max_convoi", 8);

    // Conversion des pourcentages (ex: 70% devient 0.7)
    m_seuil_remplissage_min = lire_parametre("taux_remplissage_min", 70) / 100.0;
    m_seuil_critique        = lire_parametre("seuil_critique_suppression", 10) / 100.0;

    // Poids pour le calcul du score de performance
    m_poids_alpha = lire_parametre("poids_alpha", 10);
    m_poids_beta  = lire_parametre("poids_beta", 5);
    m_poids_gamma = lire_parametre("poids_gamma", 1);

    // Initialisation de l'agenda : 1440 minutes de liberté
    m_agenda.assign(1440, false);
}

// Fonction d'aide pour chercher une clé dans la map des paramètres
int Planificateur::lire_parametre(const std::string& cle, int valeur_par_defaut) {
    auto it = m_parametres.find(cle);
    if (it != m_parametres.end()) {
        return it->second; // Clé trouvée
    }
    return valeur_par_defaut; // Clé absente, on prend le défaut
}

// ────────────────────────────────────────────────────────────────
// GESTION DE L'AGENDA (PORTAIL)
// ────────────────────────────────────────────────────────────────

// Vérifie si un créneau est entièrement libre, marges de sécurité incluses
bool Planificateur::creneau_libre(int debut, int duree) const {
    if (debut < 0 || debut + duree > 1440) return false;

    // On calcule la zone à vérifier en incluant l'espacement obligatoire
    int marge_debut = debut - m_espacement_min;
    if (marge_debut < 0) marge_debut = 0;

    int marge_fin = debut + duree + m_espacement_min;
    if (marge_fin > 1440) marge_fin = 1440;

    // Si une seule minute est déjà occupée dans cette zone, le créneau est refusé
    for (int t = marge_debut; t < marge_fin; ++t) {
        if (m_agenda[t]) return false;
    }
    
    // Vérification des travaux ou plages interdites
    if (chevauche_plage_interdite(debut, debut + duree)) return false;

    return true;
}

// Vérifie si le créneau tombe pendant une fermeture planifiée de la gare
bool Planificateur::chevauche_plage_interdite(int debut, int fin) const {
    for (size_t i = 0; i < m_plages.size(); ++i) {
        const auto& plage = m_plages[i];
        if (debut < plage.get_fin() && fin > plage.get_debut()) {
            return true; // Il y a un chevauchement
        }
    }
    return false;
}

// Bloque les minutes sur l'agenda
void Planificateur::reserver_creneau(int debut, int duree) {
    for (int t = debut; t < debut + duree; ++t) {
        m_agenda[t] = true;
    }
}

// Libère les minutes sur l'agenda
void Planificateur::liberer_creneau(int debut, int duree) {
    for (int t = debut; t < debut + duree; ++t) {
        m_agenda[t] = false;
    }
}

// Cherche le premier instant disponible à partir de t_min
int Planificateur::trouver_creneau(int t_min, int duree) const {
    for (int t = t_min; t + duree <= 1440; ++t) {
        if (creneau_libre(t, duree)) {
            return t; // Créneau trouvé !
        }
    }
    return -1; // Aucune place disponible
}

// ────────────────────────────────────────────────────────────────
// FORMATION DES CONVOIS
// ────────────────────────────────────────────────────────────────

std::vector<Convoi> Planificateur::former_convois_sortie(int id_dest,
                                                         int& passagers_restants,
                                                         std::vector<Voiture*>& voitures_disponibles,
                                                         std::unordered_map<Voiture*, int>& historiques)
{
    std::vector<Convoi> convois_crees;
    std::vector<Voiture*> candidats;
    
    // 1. Filtrer les voitures valides pour cette destination
    for (size_t i = 0; i < voitures_disponibles.size(); ++i) 
    {
        Voiture* v = voitures_disponibles[i];
        if (v && v->get_etat() == EtatVoiture::EN_ATTENTE_GARE && v->get_destination() == id_dest) 
        {
            candidats.push_back(v);
        }
    }

    // 2. Trier par places occupées décroissantes (les plus remplies d'abord)
    std::sort(candidats.begin(), candidats.end(), [](Voiture* a, Voiture* b) {
        int occupes_a = a->get_places_max() - a->get_places_libres();
        int occupes_b = b->get_places_max() - b->get_places_libres();
        return occupes_a > occupes_b;
    });

    // Marqueurs pour les voitures déjà utilisées dans un autre convoi
    std::vector<bool> deja_prise(candidats.size(), false);

    // 3. Création des convois et remplissage
    while (passagers_restants > 0) {
        Convoi convoi(m_prochain_id_convoi++, TypeConvoi::SORTIE);

        for (size_t i = 0; i < candidats.size(); ++i) 
        {
            if (deja_prise[i] || convoi.est_plein()) continue;

            Voiture* v = candidats[i];
            int libres = v->get_places_libres();

            // Calcul du taux de remplissage prévisionnel
            int places_occupees_actuelles = v->get_places_max() - libres;
            int max_passagers_ajoutables = std::min(libres, passagers_restants);
            double taux_previsionnel = static_cast<double>(places_occupees_actuelles + max_passagers_ajoutables) 
                                       / v->get_places_max();

            // Si même après avoir ajouté tous les passagers restants la voiture reste trop vide, on l'ignore
            if (taux_previsionnel < m_seuil_remplissage_min) {
                continue;
            }

            // Embarquement des passagers
            int a_embarquer = std::min(libres, passagers_restants);
            v->embarquer(a_embarquer);
            historiques[v] += a_embarquer; // pour rollback éventuel
            passagers_restants -= a_embarquer;
            
            convoi.ajouter_voiture(v);
            deja_prise[i] = true;
        }

        if (convoi.get_taille() > 0) {
            convois_crees.push_back(std::move(convoi));
        } else {
            break; // plus de voitures disponibles répondant aux critères
        }
    }
    return convois_crees;
}

// Même logique que ci-dessus, mais pour les retours des provinces vers la gare
std::vector<Convoi> Planificateur::former_convois_retour(int id_province,
                                                         int& passagers_restants,
                                                         std::vector<Voiture*>& voitures_disponibles,
                                                         std::unordered_map<Voiture*, int>& historiques)
{
    std::vector<Convoi> convois_crees;
    std::vector<Voiture*> candidats;
    
    for (size_t i = 0; i < voitures_disponibles.size(); ++i) {
        Voiture* v = voitures_disponibles[i];
        if (v && v->get_etat() == EtatVoiture::EN_ATTENTE_STATION && v->get_destination() == 0) {
            candidats.push_back(v);
        }
    }

    std::sort(candidats.begin(), candidats.end(), [](Voiture* a, Voiture* b) {
        return (a->get_places_max() - a->get_places_libres()) > (b->get_places_max() - b->get_places_libres());
    });

    std::vector<bool> deja_prise(candidats.size(), false);

    while (passagers_restants > 0) {
        Convoi convoi(m_prochain_id_convoi++, TypeConvoi::ENTREE);

        for (size_t i = 0; i < candidats.size(); ++i) {
            if (deja_prise[i] || convoi.est_plein()) continue;

            Voiture* v = candidats[i];
            int libres = v->get_places_libres();

            // Calcul du taux de remplissage prévisionnel
            int places_occupees_actuelles = v->get_places_max() - libres;
            int max_passagers_ajoutables = std::min(libres, passagers_restants);
            double taux_previsionnel = static_cast<double>(places_occupees_actuelles + max_passagers_ajoutables) 
                                       / v->get_places_max();

            if (taux_previsionnel < m_seuil_remplissage_min) {
                continue; // la voiture resterait trop vide, on passe à la suivante
            }

            int a_embarquer = std::min(libres, passagers_restants);
            v->embarquer(a_embarquer);
            historiques[v] += a_embarquer;
            passagers_restants -= a_embarquer;
            
            convoi.ajouter_voiture(v);
            deja_prise[i] = true;
        }

        if (convoi.get_taille() > 0) {
            convois_crees.push_back(std::move(convoi));
        } else {
            break;
        }
    }
    return convois_crees;
}

// ────────────────────────────────────────────────────────────────
// PLANIFICATION GLOBALE (ALGORITHME PRINCIPAL)
// ────────────────────────────────────────────────────────────────
bool Planificateur::planifier_global(
    const std::unordered_map<int, int>& demande_depart,
    const std::unordered_map<int, int>& demande_retour,
    std::vector<Voiture*>& voitures_gare,
    const std::unordered_map<int, std::vector<Voiture*>>& voitures_par_province,
    int temps_courant)
{
    // Réinitialisation complète
    m_agenda.assign(1440, false);
    m_convois_sortie.clear();
    m_convois_entree.clear();

    std::vector<Convoi> tous_les_convois;
    std::unordered_map<Voiture*, int> historiques_embarquements;

    // 1. Génération des convois de Départ (Sorties)
    for (auto paire : demande_depart) 
    {
        int id_dest = paire.first;
        int nb_passagers = paire.second;
        if (nb_passagers <= 0) continue;

        int restants = nb_passagers;
        auto convois = former_convois_sortie(id_dest, restants, voitures_gare, historiques_embarquements);
        
        for (size_t i = 0; i < convois.size(); ++i) 
        {
            int t_min = std::max(temps_courant + m_delai_achat_min, m_debut_journee);
            convois[i].set_horaire_prevue(t_min);
            tous_les_convois.push_back(std::move(convois[i]));
        }
    }

    // 2. Génération des convois de Retour (Entrées)
    for (auto paire : demande_retour) 
    {
        int id_prov = paire.first;
        int nb_passagers = paire.second;
        if (nb_passagers <= 0) continue;

        auto it_prov = voitures_par_province.find(id_prov);
        if (it_prov == voitures_par_province.end()) continue;

        std::vector<Voiture*> voitures_prov = it_prov->second;
        int restants = nb_passagers;
        auto convois = former_convois_retour(id_prov, restants, voitures_prov, historiques_embarquements);
        
        for (size_t i = 0; i < convois.size(); ++i) 
        {
            int duree_trajet = m_destinations.at(id_prov).get_duree_trajet();
            int t_min = std::max(temps_courant + m_delai_achat_min, m_debut_journee) + duree_trajet;
            convois[i].set_horaire_prevue(t_min);
            tous_les_convois.push_back(std::move(convois[i]));
        }
    }

    // 3. Tri des convois (les plus chargés d'abord pour maximiser le flux de passagers)
    std::sort(tous_les_convois.begin(), tous_les_convois.end(), [](const Convoi& a, const Convoi& b) {
        int pass_a = 0, pass_b = 0;
        for (auto* v : a.get_voitures()) 
        {
            pass_a += (v->get_places_max() - v->get_places_libres());
        }

        for (auto* v : b.get_voitures()) 
        {
            pass_b += (v->get_places_max() - v->get_places_libres());
        }
        if (pass_a != pass_b) return pass_a > pass_b; // trie decroissant
        // si pass_a == pass_b
        return a.get_horaire_prevue() < b.get_horaire_prevue();// tri croissant le convoi dont l'horaire est plus tot passe devant
    });

    // 4. Placement sur la grille horaire avec système de réparation si conflit
    std::vector<Convoi> convois_places;
    for (size_t i = 0; i < tous_les_convois.size(); ++i) {
        Convoi& convoi = tous_les_convois[i];
        int duree = convoi.get_taille() * m_franchissement_par_voiture;
        int t = trouver_creneau(convoi.get_horaire_prevue(), duree);
        
        if (t != -1)
        {
            reserver_creneau(t, duree);
            convoi.set_horaire_prevue(t);
            convoi.set_etat(EtatConvoi::PRET);
            convois_places.push_back(std::move(convoi));
        } else {
            // Pas de place directe -> on tente de pousser un autre convoi pour faire de la place
            if (!reparer_et_inserer(convoi, convois_places, temps_courant)) {
                // Échec total de placement : on fait redescendre les passagers pour libérer la voiture
                for (auto* v : convoi.get_voitures()) {
                    if (historiques_embarquements.count(v) > 0) // on genre les passage conserner et on leur demande de choisir de nouveau creneaux
                    {
                        v->debarquer(historiques_embarquements[v]); 
                    }
                }
            }
        }
    }

    // Répartition finale dans les listes membres
    for (size_t i = 0; i < convois_places.size(); ++i) {
        if (convois_places[i].get_type() == TypeConvoi::SORTIE) 
        {
            m_convois_sortie.push_back(std::move(convois_places[i]));
        } else {
            m_convois_entree.push_back(std::move(convois_places[i]));
        }
    }

    // Phase d'amélioration locale (Fusions / Ajustements de minutes)
    ameliorer_plan_global(temps_courant);
    return true;
}

// ────────────────────────────────────────────────────────────────
// MÉTHODE DE RÉPARATION (RÉSOLUTION DES CONFLITS)
// ────────────────────────────────────────────────────────────────
bool Planificateur::reparer_et_inserer(Convoi& nouveau, std::vector<Convoi>& places, int temps_courant) {
    int duree_necessaire = nouveau.get_taille() * m_franchissement_par_voiture;
    int t_min_nouveau = nouveau.get_horaire_prevue(); 

    // On parcourt les convois déjà installés pour voir si bouger l'un d'eux libère assez d'espace
    for (size_t i = 0; i < places.size(); ++i) 
    {
        Convoi& c_deplace = places[i];
        int ancien_debut = c_deplace.get_horaire_prevue();
        int duree_deplace = c_deplace.get_taille() * m_franchissement_par_voiture;

        // Calcul de la borne d'attente minimale pour ce convoi
        int t_min_deplace;
        if (c_deplace.get_type() == TypeConvoi::SORTIE) {
            t_min_deplace = std::max(temps_courant + m_delai_achat_min, m_debut_journee);
        } else {
            int id_prov = c_deplace.get_voitures().front()->get_destination();
            int duree_trajet = m_destinations.at(id_prov).get_duree_trajet();
            t_min_deplace = std::max(temps_courant + m_delai_achat_min, m_debut_journee) + duree_trajet;
        }

        // On teste des petits décalages autour de sa position actuelle (de -60 min à +120 min)
        for (int dec = -60; dec <= 120; ++dec) 
        {
            if (dec == 0) continue;
            int nouveau_debut = ancien_debut + dec;
            if (nouveau_debut < t_min_deplace || (nouveau_debut + duree_deplace) > 1440) continue;

            // Test virtuel en libérant temporairement l'agenda
            liberer_creneau(ancien_debut, duree_deplace);
            
            if (creneau_libre(nouveau_debut, duree_deplace)) {
                reserver_creneau(nouveau_debut, duree_deplace);
                
                // Le déplacement libère-t-il la place pour notre nouveau convoi ?
                int t = trouver_creneau(t_min_nouveau, duree_necessaire);
                if (t != -1) {
                    reserver_creneau(t, duree_necessaire);
                    c_deplace.set_horaire_prevue(nouveau_debut);
                    nouveau.set_horaire_prevue(t);
                    nouveau.set_etat(EtatConvoi::PRET);
                    places.push_back(std::move(nouveau));
                    return true; // Réparation réussie !
                } else {
                    liberer_creneau(nouveau_debut, duree_deplace);
                }
            }
            // Annulation du test virtuel, on remet l'agenda en place
            reserver_creneau(ancien_debut, duree_deplace);
        }
    }
    return false; // Pas de solution de secours trouvée
}

// ────────────────────────────────────────────────────────────────
// OPTIMISATION GLOBALE DU PLANNING
// ────────────────────────────────────────────────────────────────
void Planificateur::ameliorer_plan_global(int temps_courant) {
    double meilleur_score = calculer_score(m_convois_sortie, m_convois_entree, temps_courant);
    bool progression = true;

    while (progression) {
        progression = false;

        // --- STEP 1 : FUSION DES CONVOIS TRÈS PROCHES ET SEMBLABLES ---
        // On essaie de fusionner deux convois de sorties allant au même endroit
        for (size_t i = 0; i < m_convois_sortie.size(); ++i) {
            for (size_t j = i + 1; j < m_convois_sortie.size(); ++j) {
                Convoi& c1 = m_convois_sortie[i];
                Convoi& c2 = m_convois_sortie[j];

                if (c1.get_voitures().front()->get_destination() != c2.get_voitures().front()->get_destination()) continue;
                if (c1.get_taille() + c2.get_taille() > m_taille_max_convoi) continue;
// si meme type ??

                int h1 = c1.get_horaire_prevue(), h2 = c2.get_horaire_prevue();
                int d1 = c1.get_taille() * m_franchissement_par_voiture;
                int d2 = c2.get_taille() * m_franchissement_par_voiture;

                liberer_creneau(h1, d1);
                liberer_creneau(h2, d2);

                Convoi fusion(m_prochain_id_convoi++, c1.get_type());
                for (auto* v : c1.get_voitures()) fusion.ajouter_voiture(v);
                for (auto* v : c2.get_voitures()) fusion.ajouter_voiture(v);

                int nouv_dur = fusion.get_taille() * m_franchissement_par_voiture;
                int t = trouver_creneau(std::min(h1, h2), nouv_dur);

                if (t != -1) 
                {
                    reserver_creneau(t, nouv_dur);
                    fusion.set_horaire_prevue(t);
                    fusion.set_etat(EtatConvoi::PRET);

                    // Sauvegarde de secours (Backup)
                    std::vector<Convoi> backup = m_convois_sortie;
                    m_convois_sortie.erase(m_convois_sortie.begin() + j);
                    m_convois_sortie.erase(m_convois_sortie.begin() + i);
                    m_convois_sortie.push_back(std::move(fusion));

                    double nouv_score = calculer_score(m_convois_sortie, m_convois_entree, temps_courant);
                    if (nouv_score > meilleur_score) {
                        meilleur_score = nouv_score;
                        progression = true;
                        break; // On relance la boucle d'amélioration globale
                    } else {
                        // Moins bon score : Rollback complet
                        liberer_creneau(t, nouv_dur);
                        m_convois_sortie = std::move(backup);
                        reserver_creneau(h1, d1);
                        reserver_creneau(h2, d2);
                    }
                } else {
                    reserver_creneau(h1, d1);
                    reserver_creneau(h2, d2);
                }
            }
            if (progression) break;
        }
        if (progression) continue;

        // --- STEP 2 : MICRO-DÉCALAGES INDIVIDUELS POUR OPTIMISER LE RETARD ---
        // On teste si bouger un convoi de quelques minutes à gauche ou à droite augmente notre score
        for (size_t i = 0; i < m_convois_sortie.size(); ++i) {
            Convoi& c = m_convois_sortie[i];
            int ancien_h = c.get_horaire_prevue();
            int duree = c.get_taille() * m_franchissement_par_voiture;

            for (int delta = -30; delta <= 30; ++delta) {
                if (delta == 0) continue;
                int nouv_h = ancien_h + delta;
                if (nouv_h < m_debut_journee || nouv_h + duree > 1440) continue;

                liberer_creneau(ancien_h, duree);
                if (creneau_libre(nouv_h, duree)) {
                    reserver_creneau(nouv_h, duree);
                    c.set_horaire_prevue(nouv_h);

                    double nouv_score = calculer_score(m_convois_sortie, m_convois_entree, temps_courant);
                    if (nouv_score > meilleur_score) {
                        meilleur_score = nouv_score;
                        progression = true;
                        break;
                    } else {
                        liberer_creneau(nouv_h, duree);
                        reserver_creneau(ancien_h, duree);
                        c.set_horaire_prevue(ancien_h);
                    }
                } else {
                    reserver_creneau(ancien_h, duree);
                }
            }
            if (progression) break;
        }
        if (progression) continue;

        // --- STEP 3 : SUPPRESSION DES CONVOIS "FANTÔMES" OU PRESQUE VIDES ---
        for (size_t i = 0; i < m_convois_sortie.size(); ++i) {
            Convoi& c = m_convois_sortie[i];
            int tot_pl = 0, occ_pl = 0;
            for (auto* v : c.get_voitures()) 
            {
                tot_pl += v->get_places_max();
                occ_pl += (v->get_places_max() - v->get_places_libres());
            }
            
            double taux = (tot_pl > 0) ? (double)occ_pl / tot_pl : 0.0;
            if (taux < m_seuil_critique) {
                int h = c.get_horaire_prevue();
                int d = c.get_taille() * m_franchissement_par_voiture;
                
                liberer_creneau(h, d);
                std::vector<Convoi> backup = m_convois_sortie;
                m_convois_sortie.erase(m_convois_sortie.begin() + i);
                
                double nouv_score = calculer_score(m_convois_sortie, m_convois_entree, temps_courant);
                if (nouv_score > meilleur_score) {
                    meilleur_score = nouv_score;
                    progression = true;
                    break;
                } else {
                    reserver_creneau(h, d);
                    m_convois_sortie = std::move(backup);
                }
            }
        }
    } 
}

// ────────────────────────────────────────────────────────────────
// CALCUL DU SCORE OBJECTIF
// ────────────────────────────────────────────────────────────────
double Planificateur::calculer_score(const std::vector<Convoi>& sorties,
                                      const std::vector<Convoi>& entrees,
                                      int temps_courant) const {
    int total_passagers = 0;
    double retard_total = 0.0;
    int nb_convois = static_cast<int>(sorties.size() + entrees.size());

    // Analyse des départs
    for (size_t i = 0; i < sorties.size(); ++i) {
        const Convoi& convoi = sorties[i];
        int passagers_convoi = 0;
        for (const Voiture* v : convoi.get_voitures()) {
            passagers_convoi += (v->get_places_max() - v->get_places_libres());
        }
        total_passagers += passagers_convoi;
        
        // Retard = temps réel d'envoi moins l'heure idéale désirée
        int retard = convoi.get_horaire_prevue() - (temps_courant + m_delai_achat_min);
        if (retard < 0) retard = 0;
        retard_total += retard * passagers_convoi;
    }

    // Calcul de la moyenne du retard pondéré
    double retard_moyen = 0.0;
    if (total_passagers > 0) {
        retard_moyen = retard_total / total_passagers;
    }

    // Application de la formule mathématique finale
    return (m_poids_alpha * total_passagers) - (m_poids_beta * nb_convois) - (m_poids_gamma * retard_moyen);
}

//-----------------------------------------------------
// RECUPERATION DES CONVOIS 
//-----------------------------------------------------
std::pair<std::unordered_map<int, int>, std::unordered_map<int, int>>
calculer_demande_residuelle(
    const std::unordered_map<int, int>& demande_depart_initiale,
    const std::unordered_map<int, int>& demande_retour_initiale,
    const std::vector<Convoi>& convois_sortie,
    const std::vector<Convoi>& convois_entree)
{
    // 1. Copier les demandes initiales
    std::unordered_map<int, int> residuelle_depart = demande_depart_initiale;
    std::unordered_map<int, int> residuelle_retour = demande_retour_initiale;

    // 2. Retrancher les passagers des convois de sortie
    for (const auto& convoi : convois_sortie) {
        // Toutes les voitures d'un convoi ont la même destination (garanti)
        int id_dest = convoi.get_voitures().front()->get_destination();
        int passagers_convoi = 0;
        for (const Voiture* v : convoi.get_voitures()) {
            passagers_convoi += (v->get_places_max() - v->get_places_libres());
        }

        residuelle_depart[id_dest] -= passagers_convoi;
        if (residuelle_depart[id_dest] <= 0) {
            residuelle_depart.erase(id_dest);  // enlever les clés à zéro
        }
    }

    // 3. Retrancher les passagers des convois d'entrée
    for (const auto& convoi : convois_entree) {
        // Pour un retour, la province d'origine est la position actuelle de la voiture
        // (après être arrivée en province, elle y est encore, état EN_ATTENTE_STATION)
        int id_prov = convoi.get_voitures().front()->get_position();
        int passagers_convoi = 0;
        for (const Voiture* v : convoi.get_voitures()) {
            passagers_convoi += (v->get_places_max() - v->get_places_libres());
        }

        residuelle_retour[id_prov] -= passagers_convoi;
        if (residuelle_retour[id_prov] <= 0) {
            residuelle_retour.erase(id_prov);
        }
    }

    return {residuelle_depart, residuelle_retour};
}


//-----------------------------------------------------
// RECUPERATION DES CONVOIS EXEMPLE
//-----------------------------------------------------
/*
// Au début d'un cycle de simulation (toutes les 30 minutes par exemple)
std::unordered_map<int, int> demande_depart;   // remplie par votre générateur
std::unordered_map<int, int> demande_retour;   // idem

// On conserve les demandes initiales pour le calcul du reliquat
auto dep_init = demande_depart;
auto ret_init = demande_retour;

// Planification
planificateur.planifier_global(demande_depart, demande_retour,
                               voitures_gare, voitures_par_province, temps_courant);

// Récupérer les convois
const auto& sorties = planificateur.get_convois_sortie();
const auto& entrees = planificateur.get_convois_entree();

// Calculer les passagers non placés
auto [dep_restant, ret_restant] = calculer_demande_residuelle(
    dep_init, ret_init, sorties, entrees);

// Les ajouter à la demande du prochain cycle (qui sera regénérée entre-temps)
for (auto& [dest, nb] : dep_restant) {
    demande_depart[dest] += nb;   // ou conservez-les dans une map séparée
}
for (auto& [prov, nb] : ret_restant) {
    demande_retour[prov] += nb;
}
*/