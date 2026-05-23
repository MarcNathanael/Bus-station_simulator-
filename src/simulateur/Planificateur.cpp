#include "Planificateur.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <cassert>

// ────────────────────────────────────────────────────────────────
// Constructeur
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
    // Extraction des paramètres avec valeurs par défaut si absents
    auto it = m_parametres.find("espacement_min_entre_occupation_convois");
    m_espacement_min = (it != m_parametres.end()) ? it->second : 10;

    it = m_parametres.find("duree_franchissement_voiture");
    m_franchissement_par_voiture = (it != m_parametres.end()) ? it->second : 5;

    it = m_parametres.find("duree_min_achat_avant_depart");
    m_delai_achat_min = (it != m_parametres.end()) ? it->second : 10;

    it = m_parametres.find("debut_journee");
    m_debut_journee = (it != m_parametres.end()) ? it->second : 0;

    it = m_parametres.find("fin_journee");
    m_fin_journee = (it != m_parametres.end()) ? it->second : 1440;

    it = m_parametres.find("taille_max_convoi");
    m_taille_max_convoi = (it != m_parametres.end()) ? it->second : 8;

    // Seuils et poids (valeurs par défaut si non définis)
    it = m_parametres.find("taux_remplissage_min");
    m_seuil_remplissage_min = (it != m_parametres.end()) ? it->second / 100.0 : 0.2; // 20%

    it = m_parametres.find("seuil_critique_suppression");
    m_seuil_critique = (it != m_parametres.end()) ? it->second / 100.0 : 0.1; // 10%

    it = m_parametres.find("poids_alpha");
    m_poids_alpha = (it != m_parametres.end()) ? it->second : 10.0;

    it = m_parametres.find("poids_beta");
    m_poids_beta = (it != m_parametres.end()) ? it->second : 5.0;

    it = m_parametres.find("poids_gamma");
    m_poids_gamma = (it != m_parametres.end()) ? it->second : 1.0;

    // Initialisation de l'agenda à vide (taille 1440)
    m_agenda_portail.assign(1440, false);
}

// ────────────────────────────────────────────────────────────────
// Méthodes de gestion de l'agenda
// ────────────────────────────────────────────────────────────────

bool Planificateur::creneau_libre(int debut, int duree) const {
    if (debut < 0 || debut + duree > 1440) return false; // débordement
    for (int t = debut; t < debut + duree; ++t) {
        if (m_agenda_portail[t]) return false; // occupé
        // Vérification plages interdites : on regarde si l'intervalle [debut, fin[ chevauche une plage
        if (chevauche_plage_interdite(debut, debut + duree)) return false;
    }
    return true;
}

bool Planificateur::chevauche_plage_interdite(int debut, int fin) const {
    for (const auto& plage : m_plages) {
        // Chevauchement si debut < plage.fin ET fin > plage.debut
        if (debut < plage.get_fin() && fin > plage.get_debut()) {
            return true;
        }
    }
    return false;
}

void Planificateur::reserver_creneau(int debut, int duree) {
    assert(debut + duree <= 1440);
    for (int t = debut; t < debut + duree; ++t) {
        m_agenda_portail[t] = true;
    }
}

void Planificateur::liberer_creneau(int debut, int duree) {
    assert(debut + duree <= 1440);
    for (int t = debut; t < debut + duree; ++t) {
        m_agenda_portail[t] = false;
    }
}

int Planificateur::trouver_creneau(int t_min, int duree) const {
    // On limite la recherche entre t_min et la fin de journée (inclus si possible)
    for (int t = t_min; t + duree <= 1440; ++t) {
        if (creneau_libre(t, duree)) {
            return t;
        }
    }
    return -1; // aucun créneau trouvé
}

// ────────────────────────────────────────────────────────────────
// Phase 1 : Construction gloutonne des convois
// ────────────────────────────────────────────────────────────────

std::vector<Convoi> Planificateur::former_convois_sortie(int id_dest, int& nb_passagers_restants, std::vector<Voiture*>& voitures_dispos)
{
    std::vector<Convoi> convois;

    // Filtrer les voitures disponibles pour cette destination et non pleines
    std::vector<Voiture*> candidats;
    for (auto* v : voitures_dispos) {
        if (v && v->get_etat() == EtatVoiture::EN_ATTENTE_GARE
            && v->get_destination() == id_dest
            && !v->est_pleine()
            && v->get_places_libres() > 0)
        {
            candidats.push_back(v);
        }
    }

    // Trier par places libres décroissantes (les plus remplies d'abord)
    std::sort(candidats.begin(), candidats.end(),
              [](Voiture* a, Voiture* b) {
                  return a->get_places_libres() > b->get_places_libres(); // on veut décroissant(attention)
              });

    while (nb_passagers_restants > 0 && !candidats.empty()) 
    {
        // Créer un nouveau convoi
        Convoi convoi(m_prochain_id_convoi++, TypeConvoi::SORTIE);
        int places_prises = 0;

        for (auto it = candidats.begin(); it != candidats.end() && !convoi.est_plein(); ) 
        {
            Voiture* v = *it;
            // Vérifie le seuil de remplissage minimal
            int places_dispo = v->get_places_libres();
            if (places_dispo < m_seuil_remplissage_min * v->get_places_max()) {
                ++it; // voiture trop vide, on la saute
                continue;
            }

            // Embarquer le maximum possible
            int passagers_a_embarquer = std::min(places_dispo, nb_passagers_restants);
            v->embarquer(passagers_a_embarquer); // mis a jour de nombre de place
            nb_passagers_restants -= passagers_a_embarquer;
            places_prises += passagers_a_embarquer;

            convoi.ajouter_voiture(v);

            // Retirer la voiture de la liste des disponibles
            it = candidats.erase(it);
            // Note : on ne réavance pas it car erase retourne l'élément suivant

            if (convoi.est_plein()) break;
        }

        // Ne pas créer un convoi vide
        if (convoi.get_taille() > 0) {
            convois.push_back(std::move(convoi));
        } else {
            break; // plus de voitures pouvant être ajoutées
        }
    }
    return convois;
}

// ────────────────────────────────────────────────────────────────
// Planification d'un retour
// ────────────────────────────────────────────────────────────────

Convoi Planificateur::planifier_retour(const Convoi& convoi_sortie) {
    // Créer un convoi entrant avec les mêmes voitures
    Convoi convoi_retour(m_prochain_id_convoi++, TypeConvoi::ENTREE);
    for (Voiture* v : convoi_sortie.get_voitures()) {
        convoi_retour.ajouter_voiture(v);
    }

    // Calcul de l'horaire de retour théorique
    int id_dest = convoi_sortie.get_voitures().front()->get_destination();
    const Destination& dest = m_destinations.at(id_dest);
    int duree_trajet = dest.get_duree_trajet();

    int temps_dechargement = 10; // minutes (à lire depuis paramètres si besoin)
    auto it = m_parametres.find("temps_dechargement");
    if (it != m_parametres.end()) temps_dechargement = it->second;

    // Moment où la voiture arrive à la station de destination
    int t_arrivee_dest = convoi_sortie.get_horaire_prevue() + duree_trajet;
    // Moment où elle peut repartir (après déchargement/chargement)
    int t_pret_retour = t_arrivee_dest + temps_dechargement;
    // Arrivée prévue à la gare principale
    int t_retour_souhaite = t_pret_retour + duree_trajet;

    // Chercher un créneau libre pour l'arrivée du convoi (même taille)
    int duree_occupation = convoi_retour.get_taille() * m_franchissement_par_voiture;
    int t_retour = trouver_creneau(std::max(t_retour_souhaite, m_debut_journee), duree_occupation);
    if (t_retour == -1) {
        // Si pas de créneau, on retarde encore (dans la limite du possible)
        for (int decalage = 1; t_retour_souhaite + decalage + duree_occupation <= 1440; ++decalage) {
            t_retour = trouver_creneau(t_retour_souhaite + decalage, duree_occupation);
            if (t_retour != -1) break;
        }
    }

    if (t_retour != -1) {
        convoi_retour.set_horaire_prevue(t_retour);
        reserver_creneau(t_retour, duree_occupation);
        convoi_retour.set_etat(EtatConvoi::PRET);
    } else {
        // Impossible de planifier le retour, on annule le convoi entrant
        convoi_retour.liberer_voitures(); // remet les voitures en état d'attente
        convoi_retour.set_etat(EtatConvoi::TERMINE);
    }
    return convoi_retour;
}

// ────────────────────────────────────────────────────────────────
// Planification principale
// ────────────────────────────────────────────────────────────────

bool Planificateur::planifier(const std::unordered_map<int, int>& demande,
                              std::vector<Voiture*>& voitures_disponibles,
                              int temps_courant)
{
    // Réinitialiser l'agenda
    m_agenda_portail.assign(1440, false);
    m_convois_sortie.clear();
    m_convois_entree.clear();

    // Tri des destinations par ordre de demande décroissante
    std::vector<std::pair<int, int>> dests_triees(demande.begin(), demande.end());
    std::sort(dests_triees.begin(), dests_triees.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // --- Phase 1 : Construction gloutonne des convois de sortie ---
    for (const auto& [id_dest, nb_passagers] : dests_triees) {
        if (nb_passagers <= 0) continue;

        int passagers_restants = nb_passagers;
        auto convois = former_convois_sortie(id_dest, passagers_restants, voitures_disponibles);

        for (auto& convoi : convois) {
            // Calcul de la durée d'occupation
            int nb_voitures = convoi.get_taille();
            int duree_occupation = nb_voitures * m_franchissement_par_voiture;

            // Déterminer l'horaire minimum de départ
            int t_min = std::max(temps_courant + m_delai_achat_min, m_debut_journee);
            // Respecter l'espacement après le dernier convoi
            // On maintient un pointeur sur la dernière minute occupée (à faire)
            // Pour simplifier, on va chercher à partir de t_min, la recherche
            // dans trouver_creneau tiendra compte de l'agenda déjà rempli.
            int t_depart = trouver_creneau(t_min, duree_occupation);

            if (t_depart != -1) {
                convoi.set_horaire_prevue(t_depart);
                convoi.set_etat(EtatConvoi::PRET);
                reserver_creneau(t_depart, duree_occupation);
                m_convois_sortie.push_back(convoi);

                // Planifier le retour correspondant
                Convoi convoi_retour = planifier_retour(convoi);
                if (convoi_retour.get_etat() == EtatConvoi::PRET) {
                    m_convois_entree.push_back(std::move(convoi_retour));
                } else {
                    // Si le retour est impossible, annuler le départ ?
                    // Pour l'instant, on laisse le départ mais on logue
                    // TODO : gestion plus fine
                }
            } else {
                // Pas de créneau : on annule le convoi et on rembourse les passagers
                for (Voiture* v : convoi.get_voitures()) {
                    // Remettre la voiture en attente à la gare avec ses places libres actuelles
                    // Les places ont déjà été décrémentées par embarquer,
                    // il faut les restaurer (on annule l'embarquement)
                    // Pour simplifier, on pourrait vider le convoi sans libérer les places,
                    // mais ce n'est pas correct. On va réaugmenter les places.
                    int places_prises = v->get_places_max() - v->get_places_libres(); // approximation
                    // En réalité, il faut se souvenir combien de passagers avaient été embarqués.
                    // On va plutôt stocker le nombre de passagers embarqués par voiture dans le convoi.
                    // Pour l'instant, on laisse tel quel (problème connu).
                }
                // On ne l'ajoute pas aux convois planifiés.
            }
        }
    }

    // --- Phase 2 : Amélioration locale ---
    ameliorer_plan(m_convois_sortie, m_convois_entree, temps_courant);

    return true; // toujours true pour l'instant, pourrait être false si échec total
}

// ────────────────────────────────────────────────────────────────
// Phase 2 : Amélioration locale
// ────────────────────────────────────────────────────────────────

void Planificateur::ameliorer_plan(std::vector<Convoi>& convois_sortie,
                                   std::vector<Convoi>& convois_entree,
                                   int temps_courant)
{
    // Copie de la liste pour pouvoir tester des modifications
    double meilleur_score = calculer_score(convois_sortie, temps_courant);
    bool progression = true;

    while (progression) {
        progression = false;

        // --- Tentative de fusion de deux convois ---
        for (size_t i = 0; i < convois_sortie.size(); ++i) {
            for (size_t j = i + 1; j < convois_sortie.size(); ++j) {
                Convoi& c1 = convois_sortie[i];
                Convoi& c2 = convois_sortie[j];
                // Même destination et état PRET
                if (c1.get_voitures().front()->get_destination() !=
                    c2.get_voitures().front()->get_destination())
                    continue;
                if (c1.get_taille() + c2.get_taille() > m_taille_max_convoi) continue;

                // Sauvegarde des créneaux pour rollback
                int horaire1 = c1.get_horaire_prevue();
                int horaire2 = c2.get_horaire_prevue();
                int duree1 = c1.get_taille() * m_franchissement_par_voiture;
                int duree2 = c2.get_taille() * m_franchissement_par_voiture;

                // Libérer les créneaux
                liberer_creneau(horaire1, duree1);
                liberer_creneau(horaire2, duree2);

                // Créer un convoi fusionné (on déplace les voitures)
                Convoi fusion(m_prochain_id_convoi++, TypeConvoi::SORTIE);
                for (Voiture* v : c1.get_voitures()) fusion.ajouter_voiture(v);
                for (Voiture* v : c2.get_voitures()) fusion.ajouter_voiture(v);

                // Chercher un créneau pour la fusion
                int nouvelle_duree = fusion.get_taille() * m_franchissement_par_voiture;
                int t_min = std::min(horaire1, horaire2); // on essaie de placer tôt
                int nouveau_horaire = trouver_creneau(t_min, nouvelle_duree);

                if (nouveau_horaire != -1) {
                    // Appliquer temporairement
                    fusion.set_horaire_prevue(nouveau_horaire);
                    reserver_creneau(nouveau_horaire, nouvelle_duree);

                    // Remplacer les deux convois par la fusion
                    std::vector<Convoi> nouvelle_liste;
                    for (size_t k = 0; k < convois_sortie.size(); ++k) {
                        if (k != i && k != j) nouvelle_liste.push_back(convois_sortie[k]);
                    }
                    nouvelle_liste.push_back(fusion);

                    double nouveau_score = calculer_score(nouvelle_liste, temps_courant);
                    if (nouveau_score >= meilleur_score) {
                        // Accepter
                        convois_sortie = std::move(nouvelle_liste);
                        // Les retours doivent être recalculés, on simplifie en les vidant
                        convois_entree.clear();
                        for (auto& c : convois_sortie) {
                            Convoi ret = planifier_retour(c);
                            if (ret.get_etat() == EtatConvoi::PRET)
                                convois_entree.push_back(std::move(ret));
                        }
                        meilleur_score = nouveau_score;
                        progression = true;
                        break; // sortir de la boucle j, on recommencera
                    } else {
                        // Rollback
                        liberer_creneau(nouveau_horaire, nouvelle_duree);
                        reserver_creneau(horaire1, duree1);
                        reserver_creneau(horaire2, duree2);
                    }
                } else {
                    // Rollback créneaux
                    reserver_creneau(horaire1, duree1);
                    reserver_creneau(horaire2, duree2);
                }
            }
            if (progression) break; // on repart de zéro après une modification
        } // fin fusion

        // --- Tentative de décalage d'un convoi ---
        if (!progression) { // ne faire que si aucune fusion n'a été faite
            for (size_t i = 0; i < convois_sortie.size(); ++i) {
                Convoi& c = convois_sortie[i];
                int ancien_horaire = c.get_horaire_prevue();
                int duree = c.get_taille() * m_franchissement_par_voiture;
                // Essayer des décalages de -30 à +30 minutes
                for (int delta = -30; delta <= 30; ++delta) {
                    if (delta == 0) continue;
                    int nouvel_horaire = ancien_horaire + delta;
                    if (nouvel_horaire < m_debut_journee || nouvel_horaire + duree > 1440)
                        continue;
                    // Libérer l'ancien créneau
                    liberer_creneau(ancien_horaire, duree);
                    if (creneau_libre(nouvel_horaire, duree)) {
                        reserver_creneau(nouvel_horaire, duree);
                        c.set_horaire_prevue(nouvel_horaire);
                        double nouveau_score = calculer_score(convois_sortie, temps_courant);
                        if (nouveau_score >= meilleur_score) {
                            meilleur_score = nouveau_score;
                            progression = true;
                            break; // on garde ce décalage et on passe au suivant
                        } else {
                            // Rollback
                            liberer_creneau(nouvel_horaire, duree);
                            reserver_creneau(ancien_horaire, duree);
                            c.set_horaire_prevue(ancien_horaire);
                        }
                    } else {
                        // remettre l'ancien créneau
                        reserver_creneau(ancien_horaire, duree);
                    }
                }
                if (progression) break;
            }
        }

        // --- Suppression de convois sous-critiques ---
        if (!progression) {
            for (size_t i = 0; i < convois_sortie.size(); ++i) {
                Convoi& c = convois_sortie[i];
                // Calculer le taux de remplissage moyen du convoi
                int total_places = 0, total_passagers = 0;
                for (Voiture* v : c.get_voitures()) {
                    total_places += v->get_places_max();
                    total_passagers += (v->get_places_max() - v->get_places_libres());
                }
                double taux = (total_places > 0) ? (double)total_passagers / total_places : 0.0;
                if (taux < m_seuil_critique) {
                    // Sauvegarde
                    int horaire = c.get_horaire_prevue();
                    int duree = c.get_taille() * m_franchissement_par_voiture;
                    // Libérer
                    liberer_creneau(horaire, duree);
                    // Retirer le convoi de la liste
                    std::vector<Convoi> nouvelle_liste;
                    for (size_t k = 0; k < convois_sortie.size(); ++k) {
                        if (k != i) nouvelle_liste.push_back(convois_sortie[k]);
                    }
                    double nouveau_score = calculer_score(nouvelle_liste, temps_courant);
                    if (nouveau_score >= meilleur_score) {
                        convois_sortie = std::move(nouvelle_liste);
                        meilleur_score = nouveau_score;
                        progression = true;
                        break;
                    } else {
                        // Rollback
                        reserver_creneau(horaire, duree);
                    }
                }
            }
        }

    } // while progression
}

// ────────────────────────────────────────────────────────────────
// Calcul du score
// ────────────────────────────────────────────────────────────────

double Planificateur::calculer_score(const std::vector<Convoi>& convois_sortie, int temps_courant) const {
    int total_passagers = 0;
    double retard_total = 0.0;
    int nb_convois = static_cast<int>(convois_sortie.size());

    for (const auto& convoi : convois_sortie) {
        int passagers_convoi = 0;
        for (const Voiture* v : convoi.get_voitures()) {
            // Un passager est une place occupée = capacité - places libres
            passagers_convoi += (v->get_places_max() - v->get_places_libres());
        }
        total_passagers += passagers_convoi;
        int retard = std::max(0, convoi.get_horaire_prevue() - (temps_courant + m_delai_achat_min));
        retard_total += retard * passagers_convoi;
    }

    double retard_moyen = (total_passagers > 0) ? retard_total / total_passagers : 0.0;
    return m_poids_alpha * total_passagers - m_poids_beta * nb_convois - m_poids_gamma * retard_moyen;
}