#include "Planificateur.h"
#include <iostream>
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
    m_seuil_remplissage_min = lire_parametre("taux_remplissage_min", 50) / 100.0;
    m_seuil_critique        = lire_parametre("seuil_critique_suppression", 10) / 100.0;

    // Poids pour le calcul du score de performance
    m_poids_alpha = lire_parametre("poids_alpha", 10);
    m_poids_beta  = lire_parametre("poids_beta", 5);
    m_poids_gamma = lire_parametre("poids_gamma", 1);

    // Initialisation de l'agenda a la creation pas a son appel
    m_agenda.clear();
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
bool Planificateur::creneau_libre(int debut_absolu, int duree) const {
    if (debut_absolu < 0) return false;

    int marge_debut = std::max(0, debut_absolu - m_espacement_min);
    int marge_fin = debut_absolu + duree + m_espacement_min;

    // 1. Vérifier si un autre train n'occupe pas la voie en temps absolu
    for (int t = marge_debut; t < marge_fin; ++t) {
        if (m_agenda.count(t) > 0) return false; // on verifie juste la valeur de m_agenda 
    }
    
    // 2. Vérifier si le train chevauche une plage interdite (travaux)
    if (chevauche_plage_interdite(debut_absolu, debut_absolu + duree)) return false;

    return true; // Le créneau est parfait !
}
// Vérifie si le créneau tombe pendant une fermeture planifiée de la gare
/*
bool Planificateur::chevauche_plage_interdite(int debut, int fin) const 
{
    for (size_t i = 0; i < m_plages.size(); ++i) {
        const auto& plage = m_plages[i];
        // 
        if (debut < plage.get_fin() && fin > plage.get_debut()) {
            return true; // Il y a un chevauchement
        }
    }
    return false;
}

*/// Nouvel version 
bool Planificateur::chevauche_plage_interdite(int debut_absolu, int fin_absolu) const {
    // On teste chaque minute de l'intervalle demandé
    for (int t = debut_absolu; t < fin_absolu; ++t) {
        for (const auto& plage : m_plages) {
            if (plage.contient(t)) {
                return true; // Une des minutes du trajet tombe sur une plage interdite
            }
        }
    }
    return false;
}


// on insert directement l'heure dans m_agenda pas de bool
void Planificateur::reserver_creneau(int debut_absolu, int duree) {
    for (int t = debut_absolu; t < debut_absolu + duree; ++t) {
        m_agenda.insert(t); // On ajoute la minute exacte (ex: 2890)
    }
}

// Libère les minutes sur l'agenda
void Planificateur::liberer_creneau(int debut_absolu, int duree) {
    for (int t = debut_absolu; t < debut_absolu + duree; ++t) {
        m_agenda.erase(t); // On retire la minute
    }
}

// Cherche le premier instant disponible à partir de t_min
// Heure circulaire
// il ne cherche plus jusqu'a 1440 , il cherche dans une fenetre de 14440min 
int Planificateur::trouver_creneau(int t_min_absolu, int duree) const {
    int limite_recherche = t_min_absolu + 1440; // On fouille sur max 24h
    for (int t = t_min_absolu; t < limite_recherche; ++t) {
        if (creneau_libre(t, duree)) {
            return t; 
        }
    }
    return -1; 
}

void Planificateur::nettoyer_convois_passes(double temps_continu) 
{
    // 1. Nettoyer les départs (SORTIE)
    auto it_sortie = m_convois_sortie.begin();
    while (it_sortie != m_convois_sortie.end()) {
        int duree_trajet = m_destinations.count(it_sortie->get_id_region()) ? m_destinations.at(it_sortie->get_id_region()).get_duree_trajet() : 0;
        int fin_trajet = it_sortie->get_horaire_prevue() + duree_trajet + (it_sortie->get_taille() * m_franchissement_par_voiture);
        
        bool en_cours = (it_sortie->get_etat() == EtatConvoi::EN_TRANSIT || it_sortie->get_etat() == EtatConvoi::TERMINE);
        // On détruit le convoi de la mémoire SEULEMENT quand il est arrivé à destination
        // ATTENTION : On ne libère PLUS l'agenda ici, c'est le Simulateur qui le fait au moment du franchissement !
        if (en_cours && fin_trajet < temps_continu) {
            it_sortie = m_convois_sortie.erase(it_sortie);
        } else {
            ++it_sortie;
        }
    }

    // 2. Nettoyer les retours (ENTREE)
    auto it_entree = m_convois_entree.begin();
    while (it_entree != m_convois_entree.end()) {
        int fin_trajet = it_entree->get_horaire_prevue() + (it_entree->get_taille() * m_franchissement_par_voiture);
        
        bool en_cours = (it_entree->get_etat() == EtatConvoi::EN_TRANSIT || it_entree->get_etat() == EtatConvoi::TERMINE);
        if (en_cours && fin_trajet < temps_continu) {
            it_entree = m_convois_entree.erase(it_entree);
        } else {
            ++it_entree;
        }
    }
}
// ────────────────────────────────────────────────────────────────
// FORMATION DES CONVOIS
// ────────────────────────────────────────────────────────────────
std::vector<Convoi> Planificateur::former_convois_sortie(
    int id_dest, 
    int& passagers_urgents, 
    int& passagers_standards, 
    std::vector<Voiture*>& voitures_disponibles, 
    std::unordered_map<Voiture*, int>& historiques)
{
    std::vector<Convoi> convois_crees;
    std::vector<Voiture*> candidats;

    for (Voiture* v : voitures_disponibles) {
        if (v && v->get_etat() == EtatVoiture::EN_ATTENTE_GARE) {
            if (v->get_passagers() == 0 || v->get_destination() == id_dest) {
                candidats.push_back(v);
            }
        }
    }

    int total_demande = passagers_urgents + passagers_standards;
    if (candidats.empty() || total_demande == 0) return convois_crees;

    std::sort(candidats.begin(), candidats.end(), [](Voiture* a, Voiture* b) {
        return a->get_passagers() > b->get_passagers();
    });

    int places_premiere_voiture = candidats[0]->get_places_max();
    
    // JUSTIFICATION : Urgence OU seuil de remplissage normal (50% d'une voiture)
    bool justification_urgence = (passagers_urgents > 0);
    bool justification_remplissage = (total_demande >= static_cast<int>(places_premiere_voiture * m_seuil_remplissage_min));

    if (!justification_urgence && !justification_remplissage) {
        return convois_crees;
    }

    Convoi convoi(m_prochain_id_convoi++, TypeConvoi::SORTIE, m_taille_max_convoi);
    convoi.set_id_region(id_dest);
    bool convoi_a_urgence = false;

    for (size_t i = 0; i < candidats.size(); ++i) {
        if (convoi.est_plein()) break;
        Voiture* v = candidats[i];

        int total_a_placer = passagers_urgents + passagers_standards;
        int max_ajoutables = std::min(v->get_places_libres(), total_a_placer);
        int urgents_ajoutes = 0;

        if (max_ajoutables > 0) {
            urgents_ajoutes = std::min(max_ajoutables, passagers_urgents);
            if (v->embarquer(max_ajoutables)) {
                passagers_urgents -= urgents_ajoutes;
                passagers_standards -= (max_ajoutables - urgents_ajoutes);
                historiques[v] += max_ajoutables;
                if (v->get_destination() != id_dest) v->set_destination(id_dest);
                if (urgents_ajoutes > 0) convoi_a_urgence = true;
            }
        }
        
        // RÈGLE ÉCONOMIQUE : On ajoute la voiture UNIQUEMENT si elle a des passagers à bord.
        // Fini les convois gonflés de voitures vides.
        if (v->get_passagers() > 0) {
            convoi.ajouter_voiture(v);
        }
    }

    if (convoi.get_taille() > 0) {
        convoi.set_contient_urgence(convoi_a_urgence);
        convois_crees.push_back(std::move(convoi));
    }

    return convois_crees;
}

// ────────────────────────────────────────────────────────────────
// FORMATION DES CONVOIS D'ENTRÉE / RETOUR
// ────────────────────────────────────────────────────────────────
std::vector<Convoi> Planificateur::former_convois_retour(
    int id_prov, 
    int& passagers_urgents, 
    int& passagers_standards, 
    std::vector<Voiture*>& voitures_disponibles, 
    std::unordered_map<Voiture*, int>& historiques,
    bool gare_pauvre)
{
    std::vector<Convoi> convois_crees;
    std::vector<Voiture*> candidats;

    for (Voiture* v : voitures_disponibles) {
        if (v && v->get_etat() == EtatVoiture::EN_ATTENTE_STATION) {
            candidats.push_back(v);
        }
    }

    int total_demande = passagers_urgents + passagers_standards;
    if (candidats.empty()) return convois_crees;

    std::sort(candidats.begin(), candidats.end(), [](Voiture* a, Voiture* b) {
        return a->get_passagers() > b->get_passagers();
    });

    int places_premiere_voiture = candidats[0]->get_places_max();
    
    // JUSTIFICATION : Urgence, OU seuil normal, OU gare pauvre pour ramener les voitures
    bool justification_urgence = (passagers_urgents > 0);
    bool justification_remplissage = (total_demande >= static_cast<int>(places_premiere_voiture * m_seuil_remplissage_min));
    bool justification_assouplie = (total_demande >= static_cast<int>(places_premiere_voiture * 0.25));
    bool justification_forcee = gare_pauvre;

    if (!justification_urgence && !justification_remplissage && !justification_assouplie && !justification_forcee) {
        return convois_crees;
    }

    Convoi convoi(m_prochain_id_convoi++, TypeConvoi::ENTREE, m_taille_max_convoi);
    convoi.set_id_region(id_prov);
    bool convoi_a_urgence = false;

    for (size_t i = 0; i < candidats.size(); ++i) {
        if (convoi.est_plein()) break;
        Voiture* v = candidats[i];

        int total_a_placer = passagers_urgents + passagers_standards;
        int max_ajoutables = std::min(v->get_places_libres(), total_a_placer);
        int urgents_ajoutes = 0;

        if (max_ajoutables > 0) {
            urgents_ajoutes = std::min(max_ajoutables, passagers_urgents);
            if (v->embarquer(max_ajoutables)) {
                passagers_urgents -= urgents_ajoutes;
                passagers_standards -= (max_ajoutables - urgents_ajoutes);
                historiques[v] += max_ajoutables;
                if (v->get_destination() != 0) v->set_destination(0); 
                if (urgents_ajoutes > 0) convoi_a_urgence = true;
            }
        }
        
        // CORRECTION MAJEURE : On ajoute la voiture si elle a des passagers, OU si la gare est pauvre.
        // Si la gare est pauvre, on renvoie TOUTES les voitures vides pour la réapprovisionner.
        if (v->get_passagers() > 0 || justification_forcee) {
            convoi.ajouter_voiture(v);
        }
    }
    if (convoi.get_taille() > 0) {
        // CORRECTION CRITIQUE : On ne marque plus les convois de retour vides comme urgents.
        // L'urgence est réservée aux passagers critiques. Un convoi vide aura convoi_a_urgence = false.
        // Il sera donc placé APRÈS les convois remplis de passagers grâce au tri multi-critères.
        convoi.set_contient_urgence(convoi_a_urgence);
        convois_crees.push_back(std::move(convoi));
    }
    
    return convois_crees;
}

// ────────────────────────────────────────────────────────────────
// PLANIFICATION GLOBALE (ALGORITHME PRINCIPAL)
// ────────────────────────────────────────────────────────────────
bool Planificateur::planifier_global(
    std::unordered_map<int, int>& demande_depart_std,
    std::unordered_map<int, int>& demande_depart_urg,
    std::unordered_map<int, int>& demande_retour_std,
    std::unordered_map<int, int>& demande_retour_urg,
    std::vector<Voiture*>& voitures_gare,
    const std::unordered_map<int, std::vector<Voiture*>>& voitures_par_province,
    double temps_continu)
{
    std::vector<Convoi> tous_les_convois;
    std::unordered_map<Voiture*, int> historiques_embarquements;

    // --- ÉTAPE 1 : CONSOLIDATION DES DÉPARTS ---
    std::set<int> destinations_depart;
    for (auto p : demande_depart_std) destinations_depart.insert(p.first);
    for (auto p : demande_depart_urg) destinations_depart.insert(p.first);

    for (Voiture* v : voitures_gare) {
        if (v && v->get_passagers() > 0 && v->get_destination() != 0) {
            destinations_depart.insert(v->get_destination());
        }
    }

    for (int id_dest : destinations_depart) 
    {
        if (id_dest == 0) continue;

        int& pass_std = demande_depart_std[id_dest]; // Référence directe !
        int& pass_urg = demande_depart_urg[id_dest];

        auto convois = former_convois_sortie(id_dest, pass_urg, pass_std, voitures_gare, historiques_embarquements);
        
        for (size_t i = 0; i < convois.size(); ++i) {
            int t_min = static_cast<int>(std::max(temps_continu + m_delai_achat_min, static_cast<double>(m_debut_journee)));
            convois[i].set_horaire_prevue(t_min);
            tous_les_convois.push_back(std::move(convois[i]));
        }
    }

    // --- ÉTAPE 2 : CONSOLIDATION DES RETOURS ---
    std::set<int> provinces_retour;
    for (auto p : demande_retour_std) provinces_retour.insert(p.first);
    for (auto p : demande_retour_urg) provinces_retour.insert(p.first);

    for (const auto& paire : voitures_par_province) {
        for (Voiture* v : paire.second) {
            if (v && v->get_etat() == EtatVoiture::EN_ATTENTE_STATION) {
                provinces_retour.insert(paire.first);
                break;
            }
        }
    }

    // CORRECTION : On ne déclenche le retour forcé que si la gare est totalement vide, 
    // pas juste si elle a moins de 8 voitures. Sinon, on ramène des voitures vides 
    // alors qu'on en a déjà 7 sur place pour évacuer les passagers.
    bool gare_pauvre = voitures_gare.empty();

    for (int id_prov : provinces_retour) 
    {
        if (id_prov == 0) continue;

        int& pass_std = demande_retour_std[id_prov]; // Référence directe !
        int& pass_urg = demande_retour_urg[id_prov];

        auto it_prov = voitures_par_province.find(id_prov);
        if (it_prov == voitures_par_province.end()) continue;

        std::vector<Voiture*> voitures_prov = it_prov->second;
        auto convois = former_convois_retour(id_prov, pass_urg, pass_std, voitures_prov, historiques_embarquements, gare_pauvre);
        
        for (size_t i = 0; i < convois.size(); ++i) {
            int duree_trajet = m_destinations.at(id_prov).get_duree_trajet();
            int t_min = static_cast<int>(std::max(temps_continu + m_delai_achat_min,static_cast<double>(m_debut_journee))) + duree_trajet;
            convois[i].set_horaire_prevue(t_min);
            tous_les_convois.push_back(std::move(convois[i]));
        }
    }

    // 3. TRI DES CONVOIS
    std::sort(tous_les_convois.begin(), tous_les_convois.end(), [](const Convoi& a, const Convoi& b) {
        if (a.contient_urgence() != b.contient_urgence()) return a.contient_urgence() > b.contient_urgence();
        int pass_a = 0, pass_b = 0;
        for (auto* v : a.get_voitures()) pass_a += (v->get_places_max() - v->get_places_libres());
        for (auto* v : b.get_voitures()) pass_b += (v->get_places_max() - v->get_places_libres());
        if (pass_a != pass_b) return pass_a > pass_b;
        return a.get_horaire_prevue() < b.get_horaire_prevue();
    });

    // 4. Placement sur la grille horaire
    std::vector<Convoi> convois_places; 
    for (size_t i = 0; i < tous_les_convois.size(); ++i) 
    {
        Convoi& convoi = tous_les_convois[i];
        int duree = convoi.get_taille() * m_franchissement_par_voiture;
        int t = trouver_creneau(convoi.get_horaire_prevue(), duree);
        
        if (t != -1)
        {
            reserver_creneau(t, duree); 
            convoi.set_horaire_prevue(t);
            convoi.set_etat(EtatConvoi::PRET);
            convois_places.push_back(std::move(convoi)); 
        } 
        else 
        {
            if (!reparer_et_inserer(convoi, convois_places, temps_continu)) 
            {
                for (auto* v : convoi.get_voitures()) {
                    if (historiques_embarquements.count(v) > 0) {
                        v->debarquer(historiques_embarquements[v]);
                    }
                    if (convoi.get_type() == TypeConvoi::SORTIE) {
                        v->set_etat(EtatVoiture::EN_ATTENTE_GARE);
                    } else {
                        v->set_etat(EtatVoiture::EN_ATTENTE_STATION);
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < convois_places.size(); ++i) {
        if (convois_places[i].get_type() == TypeConvoi::SORTIE) 
        {
            m_convois_sortie.push_back(std::move(convois_places[i]));
        } else {
            m_convois_entree.push_back(std::move(convois_places[i]));
        }
    }

    ameliorer_plan_global(temps_continu);
    return true;
}

// ────────────────────────────────────────────────────────────────
// SYSTEME DE REPARATION (VERSION CORRIGÉE)
// ────────────────────────────────────────────────────────────────
bool Planificateur::reparer_et_inserer(Convoi& nouveau, std::vector<Convoi>& places, double temps_continu) {
    int duree_necessaire = nouveau.get_taille() * m_franchissement_par_voiture;
    int t_min_nouveau = nouveau.get_horaire_prevue(); 

    for (size_t i = 0; i < places.size(); ++i) 
    {
        Convoi& c_deplace = places[i];

        // RÈGLE D'URGENCE ABSOLUE : On ne perturbe JAMAIS un convoi de secours ou urgent
        // pour faire de la place à un autre convoi.
        if (c_deplace.contient_urgence()) continue;

        int ancien_debut = c_deplace.get_horaire_prevue();
        int duree_deplace = c_deplace.get_taille() * m_franchissement_par_voiture;
 
        int t_min_deplace;
        if (c_deplace.get_type() == TypeConvoi::SORTIE) {
            t_min_deplace = static_cast<double>(std::max(temps_continu +  m_delai_achat_min, static_cast<double>(m_debut_journee)));
        } 
        else
        {
            int id_prov = c_deplace.get_id_region(); // Remplace l'ancien accès par la voiture
            int duree_trajet = m_destinations.at(id_prov).get_duree_trajet();
            t_min_deplace = static_cast<double>(std::max(temps_continu + m_delai_achat_min, static_cast<double>(m_debut_journee))) + duree_trajet;
        }

        for (int dec = DECALAGE_MIN; dec <= DECALAGE_MAX; ++dec) 
        {
            if (dec == 0) continue;
            int nouveau_debut = ancien_debut + dec;
            
            if (nouveau_debut < t_min_deplace || (nouveau_debut + duree_deplace) > (temps_continu + 1440)) continue;

            liberer_creneau(ancien_debut, duree_deplace);
            
            if (creneau_libre(nouveau_debut, duree_deplace)) 
            {
                reserver_creneau(nouveau_debut, duree_deplace);
                
                int t = trouver_creneau(t_min_nouveau, duree_necessaire);
                if (t != -1) {
                    reserver_creneau(t, duree_necessaire);
                    c_deplace.set_horaire_prevue(nouveau_debut);
                    nouveau.set_horaire_prevue(t);
                    nouveau.set_etat(EtatConvoi::PRET);
                    places.push_back(std::move(nouveau));
                    return true; 
                } else {
                    liberer_creneau(nouveau_debut, duree_deplace);
                }
            }
            reserver_creneau(ancien_debut, duree_deplace);
        }
    }
    return false; 
}

// ────────────────────────────────────────────────────────────────
// OPTIMISATION GLOBALE DU PLANNING (VERSION CORRIGÉE)
// ────────────────────────────────────────────────────────────────
void Planificateur::ameliorer_plan_global(double temps_continu) {
    double meilleur_score = calculer_score(m_convois_sortie, m_convois_entree, temps_continu);
    bool progression = true;

    while (progression) {
        progression = false;

        // --- STEP 1 : FUSION DES CONVOIS TRÈS PROCHES ET SEMBLABLES ---
        for (size_t i = 0; i < m_convois_sortie.size(); ++i) {
            for (size_t j = i + 1; j < m_convois_sortie.size(); ++j) {
                Convoi& c1 = m_convois_sortie[i];
                Convoi& c2 = m_convois_sortie[j];

                if (c1.get_etat() != EtatConvoi::PRET || c2.get_etat() != EtatConvoi::PRET) continue;
                if (c1.get_horaire_prevue() <= temps_continu + m_delai_achat_min) continue;
                if (c2.get_horaire_prevue() <= temps_continu + m_delai_achat_min) continue;
                if (c1.get_id_region() != c2.get_id_region()) continue;
                if (c1.get_taille() + c2.get_taille() > m_taille_max_convoi) continue;

                int h1 = c1.get_horaire_prevue();
                int h2 = c2.get_horaire_prevue();
                int d1 = c1.get_taille() * m_franchissement_par_voiture;
                int d2 = c2.get_taille() * m_franchissement_par_voiture;

                liberer_creneau(h1, d1);
                liberer_creneau(h2, d2);

                Convoi fusion(m_prochain_id_convoi++, c1.get_type(), m_taille_max_convoi);
                fusion.set_contient_urgence(c1.contient_urgence() || c2.contient_urgence());

                // On rétablit l'état avant d'ajouter pour passer la sécurité de Convoi
                for (auto* v : c1.get_voitures()) {
                    v->set_etat(EtatVoiture::EN_ATTENTE_GARE);
                    fusion.ajouter_voiture(v); // ajouter_voiture repasse l'état en EN_CHARGEMENT
                }
                for (auto* v : c2.get_voitures()) {
                    v->set_etat(EtatVoiture::EN_ATTENTE_GARE);
                    fusion.ajouter_voiture(v);
                }

                int nouv_dur = fusion.get_taille() * m_franchissement_par_voiture;
                int t = trouver_creneau(std::min(h1, h2), nouv_dur);

                if (t != -1) 
                {
                    reserver_creneau(t, nouv_dur);
                    fusion.set_horaire_prevue(t);
                    fusion.set_etat(EtatConvoi::PRET);

                    std::vector<Convoi> backup = m_convois_sortie;
                    m_convois_sortie.erase(m_convois_sortie.begin() + j);
                    m_convois_sortie.erase(m_convois_sortie.begin() + i);
                    m_convois_sortie.push_back(std::move(fusion));

                    double nouv_score = calculer_score(m_convois_sortie, m_convois_entree, temps_continu);
                    if (nouv_score > meilleur_score) {
                        meilleur_score = nouv_score;
                        progression = true;
                        break; 
                    } else {
                        liberer_creneau(t, nouv_dur);
                        m_convois_sortie = std::move(backup);
                        reserver_creneau(h1, d1);
                        reserver_creneau(h2, d2);
                        
                        // CORRECTION CRITIQUE : Annuler la fusion proprement
                        // Les pointeurs de 'fusion' sont les mêmes que ceux de c1 et c2.
                        // On remet les voitures en EN_CHARGEMENT car elles réintègrent les convois c1 et c2 (restaurés dans le backup).
                        for (auto* v : m_convois_sortie[i].get_voitures()) {
                            v->set_etat(EtatVoiture::EN_CHARGEMENT);
                        }
                        for (auto* v : m_convois_sortie[j].get_voitures()) {
                            v->set_etat(EtatVoiture::EN_CHARGEMENT);
                        }
                    }
                } else {
                    reserver_creneau(h1, d1);
                    reserver_creneau(h2, d2);
                    
                    // Le créneau n'a pas été trouvé, on remet les voitures en EN_CHARGEMENT pour c1 et c2
                    for (auto* v : c1.get_voitures()) {
                        v->set_etat(EtatVoiture::EN_CHARGEMENT);
                    }
                    for (auto* v : c2.get_voitures()) {
                        v->set_etat(EtatVoiture::EN_CHARGEMENT);
                    }
                }
            }
            if (progression) break;
        }
        if (progression) continue;

        // --- STEP 2 : MICRO-DÉCALAGES INDIVIDUELS ---
        for (size_t i = 0; i < m_convois_sortie.size(); ++i) {
            Convoi& c = m_convois_sortie[i];
            int ancien_h = c.get_horaire_prevue();
            int duree = c.get_taille() * m_franchissement_par_voiture;

            if (ancien_h <= temps_continu + m_delai_achat_min) continue;
            for (int delta = -30; delta <= 30; ++delta) {
                if (delta == 0) continue;
                int nouv_h = ancien_h + delta;
                
                if (nouv_h < (temps_continu + m_delai_achat_min)) continue;
                // CORRECTION BUG 2 : Sécurité borne supérieure (Pas de débordement de journée)
                if ((nouv_h + duree) > (temps_continu + 1440)) continue;

                liberer_creneau(ancien_h, duree);
                if (creneau_libre(nouv_h, duree)) {
                    reserver_creneau(nouv_h, duree);
                    c.set_horaire_prevue(nouv_h);

                    double nouv_score = calculer_score(m_convois_sortie, m_convois_entree, temps_continu);
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

        // --- STEP 3 : SUPPRESSION DES CONVOIS SANS RENTABILITÉ ---
        // --- STEP 3 : SUPPRESSION DES CONVOIS SANS RENTABILITÉ ---
        bool convoi_supprime = false; 
        for (size_t i = 0; i < m_convois_sortie.size(); ++i) {
            Convoi& c = m_convois_sortie[i];
            
            if (c.get_etat() != EtatConvoi::PRET) continue;
            if (c.contient_urgence()) continue;

            int tot_pl = 0, occ_pl = 0;
            for (auto* v : c.get_voitures()) {
                tot_pl += v->get_places_max();
                occ_pl += (v->get_places_max() - v->get_places_libres());
            }
            
            double taux = (tot_pl > 0) ? (double)occ_pl / tot_pl : 0.0;
            if (taux < m_seuil_critique) {
                int h = c.get_horaire_prevue();
                int d = c.get_taille() * m_franchissement_par_voiture;
                
                liberer_creneau(h, d);
                std::vector<Convoi> backup = m_convois_sortie;
                std::vector<Voiture*> voitures_a_liberer = c.get_voitures(); 

                m_convois_sortie.erase(m_convois_sortie.begin() + i);
                
                double nouv_score = calculer_score(m_convois_sortie, m_convois_entree, temps_continu);
                if (nouv_score > meilleur_score) {
                    meilleur_score = nouv_score;
                    progression = true;
                    convoi_supprime = true;
                    
                    for (auto* v : voitures_a_liberer) {
                        if (v) {
                            v->debarquer_tous(); 
                            v->set_etat(EtatVoiture::EN_ATTENTE_GARE); 
                        }
                    }
                    break; // On sort pour valider la suppression
                } else {
                    reserver_creneau(h, d);
                    m_convois_sortie = std::move(backup);
                    break; //  CORRECTION CRITIQUE : Même si on annule, on casse la boucle pour éviter le SegFault !
                }
            }
        }
        if (convoi_supprime) continue; // Reprend au début du while
        // If we deleted something, the 'while(progression)' loop will run again and start the checks from the beginning
    } 
}
// ────────────────────────────────────────────────────────────────
// CALCUL DU SCORE OBJECTIF
// ────────────────────────────────────────────────────────────────
double Planificateur::calculer_score(const std::vector<Convoi>& sorties,
                                      const std::vector<Convoi>& entrees,
                                      double temps_continu) const {
    int total_passagers = 0;
    double retard_total = 0.0;
    int nb_convois = static_cast<int>(sorties.size() + entrees.size());

    // Analyse des départs
    for (size_t i = 0; i < sorties.size(); ++i) 
    {
        const Convoi& convoi = sorties[i];
        int passagers_convoi = 0;
        for (const Voiture* v : convoi.get_voitures()) 
        {
            passagers_convoi += v->get_passagers();
        }
        // total_passagers SORTIE 
        total_passagers += passagers_convoi;
        
        // Retard = temps réel d'envoi moins l'heure idéale désirée
        // horaire prevue en temps absolue et temps_continu aussu -> meme distance que circulaire
        int retard = convoi.get_horaire_prevue() - (temps_continu + m_delai_achat_min); //car ca a changer a cause de trouver_creneau
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
                               voitures_gare, voitures_par_province, temps_continu);

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


