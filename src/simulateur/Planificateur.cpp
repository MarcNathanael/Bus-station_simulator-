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
    for (int t = debut_absolu; t < fin_absolu; ++t) {
        int heure_journee = t % 1440; // Heure circulaire (0 à 1399)
        
        for (size_t i = 0; i < m_plages.size(); ++i) {
            const auto& plage = m_plages[i];
            int p_debut = plage.get_debut();
            int p_fin = plage.get_fin();

            if (p_debut < p_fin) {
                // Plage normale dans la même journée (ex: 14h00 à 16h00)
                if (heure_journee >= p_debut && heure_journee < p_fin) {
                    return true;
                }
            } else {
                // Plage qui traverse minuit ! (ex: 23h00 à 02h00)
                // C'est interdit si on est après 23h00 << OU >> avant 02h00
                if (heure_journee >= p_debut || heure_journee < p_fin) {
                    return true;
                }
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
    // 1. Nettoyer les départs
    auto it_sortie = m_convois_sortie.begin();
    while (it_sortie != m_convois_sortie.end()) {
        int fin_passage = it_sortie->get_horaire_prevue() + (it_sortie->get_taille() * m_franchissement_par_voiture);
        
        if (fin_passage < temps_continu) {
            // Le train est complètement passé, on libère l'agenda mémoire et on le supprime de la liste
            liberer_creneau(it_sortie->get_horaire_prevue(), it_sortie->get_taille() * m_franchissement_par_voiture);
            it_sortie = m_convois_sortie.erase(it_sortie);
        } else {
            ++it_sortie; // Le train est dans le futur, on le garde !
        }
    }

    // 2. Nettoyer les retours
    auto it_entree = m_convois_entree.begin();
    while (it_entree != m_convois_entree.end()) {
        int fin_passage = it_entree->get_horaire_prevue() + (it_entree->get_taille() * m_franchissement_par_voiture);
        
        if (fin_passage < temps_continu) {
            liberer_creneau(it_entree->get_horaire_prevue(), it_entree->get_taille() * m_franchissement_par_voiture);
            it_entree = m_convois_entree.erase(it_entree);
        } else {
            ++it_entree;
        }
    }
}


// ────────────────────────────────────────────────────────────────
// FORMATION DES CONVOIS
// ────────────────────────────────────────────────────────────────
// ────────────────────────────────────────────────────────────────
// FORMATION DES CONVOIS DE SORTIE (CORRIGÉE)
// ────────────────────────────────────────────────────────────────
std::vector<Convoi> Planificateur::former_convois_sortie(int id_dest,
                                                         int& passagers_urgents,
                                                         int& passagers_standards,
                                                         std::vector<Voiture*>& voitures_disponibles,
                                                         std::unordered_map<Voiture*, int>& historiques)
{
    std::vector<Convoi> convois_crees;
    std::vector<Voiture*> candidats;
    
    // 1. Filtrage des voitures à la Gare prêtes à partir vers cette destination
    for (Voiture* v : voitures_disponibles) {
        if (v && v->get_etat() == EtatVoiture::EN_ATTENTE_GARE && v->get_destination() == id_dest) {
            candidats.push_back(v);
        }
    }

    // 2. Tri décroissant : On complète d'abord les voitures déjà chargées
    std::sort(candidats.begin(), candidats.end(), [](Voiture* a, Voiture* b) {
        return a->get_passagers() > b->get_passagers();
    });

    std::vector<bool> deja_prise(candidats.size(), false);

    // 3. Remplissage glouton
    while (passagers_urgents > 0 || passagers_standards > 0) 
    {
        Convoi convoi(m_prochain_id_convoi++, TypeConvoi::SORTIE);
        convoi.set_id_region(id_dest);
        bool convoi_a_urgence = false;

        for (size_t i = 0; i < candidats.size(); ++i) 
        {
            if (deja_prise[i] || convoi.est_plein() || convoi.get_taille() >= m_taille_max_convoi) continue;
            
            int total_a_placer = passagers_urgents + passagers_standards;
            if (total_a_placer == 0) break;

            Voiture* v = candidats[i];
            int libres = v->get_places_libres();
            int max_ajoutables = std::min(libres, total_a_placer);
            
            // Calcul du taux prévisionnel avec notre fonction simplifiée
            double taux_previsionnel = static_cast<double>(v->get_passagers() + max_ajoutables) / v->get_places_max();

            bool urgence_absolue = (passagers_urgents > 0);
            if (!urgence_absolue && taux_previsionnel < m_seuil_remplissage_min) {
                continue; // Protection économique : pas assez rentable
            }

            int a_embarquer = max_ajoutables;
            int urgents_embarques = std::min(a_embarquer, passagers_urgents);
            if (urgents_embarques > 0) {
                convoi_a_urgence = true;
            }
            
            // Débitation sécurisée des files d'attente
            passagers_urgents -= urgents_embarques;
            int standards_embarques = a_embarquer - urgents_embarques;
            passagers_standards -= standards_embarques;

            // Application physique avec vérification du succès
            if (v->embarquer(a_embarquer)) {
                historiques[v] += a_embarquer; 
                convoi.ajouter_voiture(v);
                deja_prise[i] = true;
            } else {
                // Rétablissement des compteurs en cas d'échec de la structure de données
                passagers_urgents += urgents_embarques;
                passagers_standards += standards_embarques;
            }
        }

        if (convoi.get_taille() > 0) {
            convoi.set_contient_urgence(convoi_a_urgence);
            convois_crees.push_back(std::move(convoi));
        } else {
            break; // Plus aucune voiture ne peut ou ne veut accueillir les passagers restants
        }
    }
    return convois_crees;
}
// ────────────────────────────────────────────────────────────────
// FORMATION DES CONVOIS D'ENTRÉE / RETOUR (CORRIGÉE)
// ────────────────────────────────────────────────────────────────
std::vector<Convoi> Planificateur::former_convois_retour(int id_province, // Utilisé correctement maintenant !
                                                         int& passagers_urgents,
                                                         int& passagers_standards,
                                                         std::vector<Voiture*>& voitures_disponibles,
                                                         std::unordered_map<Voiture*, int>& historiques)
{
    std::vector<Convoi> convois_crees;
    std::vector<Voiture*> candidats;
    
    // 1. FILTRAGE CORRIGÉ : On cible les voitures en gare de province de "id_province"
    for (Voiture* v : voitures_disponibles) {
        if (v && v->get_etat() == EtatVoiture::EN_ATTENTE_STATION && v->get_destination() == id_province) {
            candidats.push_back(v);
        }
    }

    // 2. Tri décroissant
    std::sort(candidats.begin(), candidats.end(), [](Voiture* a, Voiture* b) {
        return a->get_passagers() > b->get_passagers();
    });

    std::vector<bool> deja_prise(candidats.size(), false);

    // 3. Remplissage glouton
    while (passagers_urgents > 0 || passagers_standards > 0) {
        Convoi convoi(m_prochain_id_convoi++, TypeConvoi::ENTREE);
        convoi.set_id_region(id_province);
        bool convoi_a_urgence = false; 

        for (size_t i = 0; i < candidats.size(); ++i) 
        {
            // AJOUT DE LA SÉCURITÉ CONVOI.EST_PLEIN() POUR LA SYMÉTRIE
            if (deja_prise[i] || convoi.est_plein() || convoi.get_taille() >= m_taille_max_convoi) continue;
            
            int total_a_placer = passagers_urgents + passagers_standards;
            if (total_a_placer == 0) break;

            Voiture* v = candidats[i];
            int libres = v->get_places_libres();
            int max_ajoutables = std::min(libres, total_a_placer);
            
            double taux_previsionnel = static_cast<double>(v->get_passagers() + max_ajoutables) / v->get_places_max();

            bool urgence_absolue = (passagers_urgents > 0);
            if (!urgence_absolue && taux_previsionnel < m_seuil_remplissage_min) {
                continue; 
            }

            int a_embarquer = max_ajoutables;
            int urgents_embarques = std::min(a_embarquer, passagers_urgents);
            if (urgents_embarques > 0) {
                convoi_a_urgence = true; 
            }
            
            passagers_urgents -= urgents_embarques;
            int standards_embarques = a_embarquer - urgents_embarques;
            passagers_standards -= standards_embarques;

            if (v->embarquer(a_embarquer)) {
                historiques[v] += a_embarquer; 
                convoi.ajouter_voiture(v);
                deja_prise[i] = true;
            } else {
                passagers_urgents += urgents_embarques;
                passagers_standards += standards_embarques;
            }
        }

        if (convoi.get_taille() > 0) {
            convoi.set_contient_urgence(convoi_a_urgence);
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
    const std::unordered_map<int, int>& demande_depart_std,
    const std::unordered_map<int, int>& demande_depart_urg,
    const std::unordered_map<int, int>& demande_retour_std,
    const std::unordered_map<int, int>& demande_retour_urg,
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

    for (int id_dest : destinations_depart) 
    {
        int pass_std = demande_depart_std.count(id_dest) ? demande_depart_std.at(id_dest) : 0;
        int pass_urg = demande_depart_urg.count(id_dest) ? demande_depart_urg.at(id_dest) : 0;

        if (pass_std <= 0 && pass_urg <= 0) continue;

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

    for (int id_prov : provinces_retour) 
    {
        // .count est tres utile avec les unordered_map , puisque la cle est unique .count retourne soit 0 soit 1 
        int pass_std = demande_retour_std.count(id_prov) ? demande_retour_std.at(id_prov) : 0;
        int pass_urg = demande_retour_urg.count(id_prov) ? demande_retour_urg.at(id_prov) : 0;

        if (pass_std <= 0 && pass_urg <= 0) continue;

        auto it_prov = voitures_par_province.find(id_prov);
        if (it_prov == voitures_par_province.end()) continue;

        std::vector<Voiture*> voitures_prov = it_prov->second;
        auto convois = former_convois_retour(id_prov, pass_urg, pass_std, voitures_prov, historiques_embarquements);
        
        for (size_t i = 0; i < convois.size(); ++i) {
            int duree_trajet = m_destinations.at(id_prov).get_duree_trajet();
            int t_min = static_cast<int>(std::max(temps_continu + m_delai_achat_min,static_cast<double>(m_debut_journee))) + duree_trajet;
            convois[i].set_horaire_prevue(t_min);
            tous_les_convois.push_back(std::move(convois[i]));
        }
    }

    // ────────────────────────────────────────────────────────────────
    // 3. TRI DES CONVOIS INTERMÉDIAIRE ( MULTI-CRITÈRES)
    // ────────────────────────────────────────────────────────────────
    std::sort(tous_les_convois.begin(), tous_les_convois.end(), [](const Convoi& a, const Convoi& b) {
        
        // CRITÈRE 1 : Priorité absolue à la présence d'une urgence médicale ou critique
        if (a.contient_urgence() != b.contient_urgence()) {
            return a.contient_urgence() > b.contient_urgence(); // true (1) passe obligatoirement avant false (0)
        }

        // CRITÈRE 2 : Si les deux partagent le même niveau d'urgence (tous deux urgents ou tous deux standards),
        // alors on applique votre tri décroissant par volume total de passagers pour maximiser le flux
        int pass_a = 0, pass_b = 0;
        for (auto* v : a.get_voitures()) {
            pass_a += (v->get_places_max() - v->get_places_libres());
        }
        for (auto* v : b.get_voitures()) {
            pass_b += (v->get_places_max() - v->get_places_libres());
        }

        if (pass_a != pass_b) {
            return pass_a > pass_b; // Trie décroissant sur la charge utile
        }

        // CRITÈRE 3 : En cas d'égalité parfaite de priorité et de charge,
        // le convoi dont l'horaire prévu est le plus ancien/tôt passe en premier (Premier Arrivé, Premier Servi)
        return a.get_horaire_prevue() < b.get_horaire_prevue();
    });

    // 4. Placement sur la grille horaire avec système de réparation si conflit
    std::vector<Convoi> convois_places; // remplie progressivement 
    for (size_t i = 0; i < tous_les_convois.size(); ++i) 
    {
        Convoi& convoi = tous_les_convois[i];
        int duree = convoi.get_taille() * m_franchissement_par_voiture;
        int t = trouver_creneau(convoi.get_horaire_prevue(), duree);// trouver vrai horaire
        
        if (t != -1)
        {
            reserver_creneau(t, duree); //agenda = true
            convoi.set_horaire_prevue(t);// set vrai horare
            convoi.set_etat(EtatConvoi::PRET);// set ret
            convois_places.push_back(std::move(convoi)); // confirmation 
        } 
        else 
        {
            // Pas de place directe -> on tente de pousser un autre convoi pour faire de la place
            // convois_place peut etre vide au debut mais il seras remplie progressivement
            if (!reparer_et_inserer(convoi, convois_places, temps_continu)) {
                // Échec total de placement : on fait redescendre les passagers pour libérer la voiture
                for (auto* v : convoi.get_voitures()) {
                    if (historiques_embarquements.count(v) > 0) // on genre les passage conserner et on leur demande de choisir de nouveau creneaux a gere dans billeterie
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

                if (c1.get_horaire_prevue() <= temps_continu + m_delai_achat_min) continue;
                if (c2.get_horaire_prevue() <= temps_continu + m_delai_achat_min) continue;
                // l'accès par la voiture par l'accès par le convoi
                if (c1.get_id_region() != c2.get_id_region()) continue;
                if (c1.get_taille() + c2.get_taille() > m_taille_max_convoi) continue;

                int h1 = c1.get_horaire_prevue();
                int h2 = c2.get_horaire_prevue();
                int d1 = c1.get_taille() * m_franchissement_par_voiture;
                int d2 = c2.get_taille() * m_franchissement_par_voiture;

                liberer_creneau(h1, d1);
                liberer_creneau(h2, d2);

                Convoi fusion(m_prochain_id_convoi++, c1.get_type());
                
                // CORRECTION BUG 1 : Transmission de l'état d'urgence au convoi fusionné
                fusion.set_contient_urgence(c1.contient_urgence() || c2.contient_urgence());

                for (auto* v : c1.get_voitures()) fusion.ajouter_voiture(v);
                for (auto* v : c2.get_voitures()) fusion.ajouter_voiture(v);

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
                    }
                } else {
                    reserver_creneau(h1, d1);
                    reserver_creneau(h2, d2);
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
        for (size_t i = 0; i < m_convois_sortie.size(); ++i) {
            Convoi& c = m_convois_sortie[i];
            
            // CORRECTION BUG 3 : Un convoi urgent a le droit de rouler à vide !
            // On l'exclut totalement de la politique de suppression économique.
            if (c.contient_urgence()) continue;

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
                
                double nouv_score = calculer_score(m_convois_sortie, m_convois_entree, temps_continu);
                if (nouv_score > meilleur_score) {
                    meilleur_score = nouv_score;
                    progression = true;
                    
                    for (auto* v : backup[i].get_voitures()) {
                        int passagers_a_bord = v->get_passagers();
                        v->debarquer(passagers_a_bord);
                    }
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
std::pair<std::unordered_map<int, int>, std::unordered_map<int, int>> 
Planificateur::calculer_demande_residuelle(const std::unordered_map<int, int>& dep_std,
                                            const std::unordered_map<int, int>& dep_urg,
                                            const std::unordered_map<int, int>& ret_std,
                                            const std::unordered_map<int, int>& ret_urg,
                                            const std::vector<Convoi>& sorties,
                                            const std::vector<Convoi>& entrees)
{
    // 1. Initialisation avec la demande totale de départ (Standard + Urgent)
    std::unordered_map<int, int> residus_depart = dep_std;
    for (const auto& paire : dep_urg) {
        residus_depart[paire.first] += paire.second;
    }

    // 2. Initialisation avec la demande totale de retour (Standard + Urgent)
    std::unordered_map<int, int> residus_retour = ret_std;
    for (const auto& paire : ret_urg) {
        residus_retour[paire.first] += paire.second;
    }

    // 3. Soustraction des clients qui ont effectivement embarqué vers la province (SORTIE)
    for (const auto& convoi : sorties) {
        for (const auto* v : convoi.get_voitures()) {
            int passagers_a_bord = v->get_passagers();
            int dest_id = v->get_destination();
            residus_depart[dest_id] -= passagers_a_bord;
        }
    }

    // 4. Soustraction des clients embarqués depuis la province vers la gare (ENTRÉE)
    for (const auto& convoi : entrees) {
        for (const auto* v : convoi.get_voitures()) {
            int passagers_a_bord = v->get_places_max() - v->get_places_libres();
            // Pour un retour, la provenance correspond à la position initiale de la voiture en province
            int province_id = v->get_position(); 
            residus_retour[province_id] -= passagers_a_bord;
        }
    }

    // 5. Sécurité : Aucun résidu ne doit être négatif suite aux arrondis ou ajustements
    for (auto& paire : residus_depart) {
        if (paire.second < 0) paire.second = 0;
    }
    for (auto& paire : residus_retour) {
        if (paire.second < 0) paire.second = 0;
    }

    return {residus_depart, residus_retour};
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

