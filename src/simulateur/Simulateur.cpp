#include "Simulateur.h"

// ============================================================================
// CONSTRUCTEUR
// ============================================================================
Simulateur::Simulateur(int id_origine,
                       std::vector<Voiture*>& flotte_globale,
                       const std::vector<PlageInterdite>& plages,
                       const std::unordered_map<int, int>& durees_trajet,
                       Billetterie& billetterie,
                       GenerateurDemandes& generateur,
                       Planificateur& planificateur,
                       int frequence_planif,
                       int duree_franchissement,
                       DatabaseManager* db,
                       DalVoiture* dal)
    : m_origine(id_origine)
    , m_temps_continue(0)
    , m_portail_occupe_jusqua(0)
    , m_duree_franchissement_par_voiture(duree_franchissement)
    , m_frequence_planif(frequence_planif)
    , m_voitures_flotte(flotte_globale)
    , m_plages_interdites(plages)
    , m_durees_trajet(durees_trajet)
    , m_billetterie(billetterie)
    , m_generateur(generateur)
    , m_planificateur(planificateur)
    , m_dbManager(db)
    , m_dalVoiture(dal)
{
}

// ============================================================================
// PERSISTANCE & VÉRIFICATIONS PHYSIQUES
// ============================================================================

bool Simulateur::en_plage_interdite(int temps) const noexcept // pas d'exeption (throw)
{
    int heure_circulaire = temps % 1440;
    for (const auto& plage : m_plages_interdites) 
    {
        int debut = plage.get_debut();
        int fin = plage.get_fin();
        
        if (debut < fin) {
            if (heure_circulaire >= debut && heure_circulaire < fin) return true;
        } 
        else 
        {
            // Plage chevauchant minuit
            if (heure_circulaire >= debut || heure_circulaire < fin) return true;
        }
    }
    return false;
}


// ============================================================================
// DATA
// ============================================================================
#include "Simulateur.h"
#include "Configuration.h" // Nécessaire pour le parsing des CSV à l'étape 2
#include <filesystem>
#include <iostream>

// ============================================================================
// CONSTRUCTEUR MIS À JOUR
// ============================================================================
Simulateur::Simulateur(int id_origine,
                       std::vector<Voiture*>& flotte_globale,
                       const std::vector<PlageInterdite>& plages,
                       const std::unordered_map<int, int>& durees_trajet,
                       Billetterie& billetterie,
                       GenerateurDemandes& generateur,
                       Planificateur& planificateur,
                       int frequence_planif,
                       int duree_franchissement,
                       DatabaseManager* dbManager,
                       DalVoiture* dalVoiture)
    : m_origine(id_origine)
    , m_temps_continue(0)
    , m_portail_occupe_jusqua(0)
    , m_duree_franchissement_par_voiture(duree_franchissement)
    , m_frequence_planif(frequence_planif)
    , m_voitures_flotte(flotte_globale)
    , m_plages_interdites(plages)
    , m_durees_trajet(durees_trajet)
    , m_billetterie(billetterie)
    , m_generateur(generateur)
    , m_planificateur(planificateur)
    , m_dbManager(dbManager)  // Initialisation du pointeur vers le gestionnaire central
    , m_dalVoiture(dalVoiture) // Initialisation de la DAL Voiture
{
}

// ============================================================================
// ÉTAPES 2 & 3 : ORCHESTRATION DU DÉMARRAGE (Méthode Statique)
// ============================================================================
bool Simulateur::orchestrer_demarrage(DatabaseManager& db, 
                                      std::vector<Voiture>& conteneur_physique, 
                                      std::vector<Voiture*>& flotte_pointeurs) 
{
    const std::string chemin_db = "data/db.sqlite";
    
    // Vérification C++17 standard de l'existence ou de la vacuité du fichier SQLite
    bool premier_lancement = !std::filesystem::exists(chemin_db) || std::filesystem::is_empty(chemin_db);

    // Initialisation de la connexion SQLite et injection des PRAGMA (WAL, SYNCHRONOUS)
    if (!db.initialiser()) {
        std::cerr << "[Orchestrateur] ERREUR : Impossible d'initialiser SQLite." << std::endl;
        return false;
    }

    DalVoiture dalVoiture(db.get_connexion());

    // --- ÉTAPE 2 : AMORÇAGE AU PREMIER LANCEMENT ---
    if (premier_lancement) {
        std::cout << "[Orchestrateur] Premier lancement détecté. Initialisation..." << std::endl;

        // 1. Création des tables via le script SQL brut
        if (!db.executer_script_sql("data/-- SQLite.sql")) {
            std::cerr << "[Orchestrateur] ERREUR : Échec de l'exécution du schéma SQL." << std::endl;
            return false;
        }

        // 2. Récupération et parsing des fichiers CSV d'origine
        Configuration config;
        if (!config.charger("requirement")) {
            std::cerr << "[Orchestrateur] ERREUR : Impossible de lire les fichiers CSV dans 'requirement/'" << std::endl;
            return false;
        }

        // 3. Écriture par lot (Transaction unique) pour déverser les CSV dans SQLite
        std::cout << "[Orchestrateur] Importation des données CSV vers SQLite..." << std::endl;
        db.commencer_transaction();
        
        for (const auto& paire : config.get_voitures()) {
            dalVoiture.inserer_voiture(paire.second);
        }
        // (Tu pourras ajouter ici l'insertion des destinations ou coopératives si nécessaire)

        db.valider_transaction();
        std::cout << "[Orchestrateur] Base de données SQLite initialisée et peuplée." << std::endl;
    } else {
        std::cout << "[Orchestrateur] Base SQLite existante détectée. Utilisation comme source de vérité." << std::endl;
    }

    // --- ÉTAPE 3 : CHARGEMENT EN MÉMOIRE RAM (CACHE) ---
    std::cout << "[Orchestrateur] Chargement du cache RAM à pleine vitesse..." << std::endl;
    
    // Remplissage du conteneur physique via un simple "SELECT * FROM voitures"
    conteneur_physique = dalVoiture.charger_tout();

    // Population du vecteur de pointeurs exigé par le simulateur
    flotte_pointeurs.clear();
    for (auto& voiture : conteneur_physique) {
        flotte_pointeurs.push_back(&voiture); // Association des adresses mémoires réelles
    }

    std::cout << "[Orchestrateur] Cache RAM prêt : " << flotte_pointeurs.size() << " véhicules chargés." << std::endl;
    return true;
}

// ============================================================================
// ÉTAPE 4 : ÉCRITURE DIFFÉRÉE (WRITE-BEHIND PAR LOTS)
// ============================================================================
void Simulateur::synchroniser_bdd() {
    // Mesure de sécurité si les composants de persistance sont absents
    if (!m_dbManager || !m_dalVoiture) return;

    // 1. Verrouillage du disque SQLite : on ouvre une transaction unique
    m_dbManager->commencer_transaction();

    // 2. On boucle sur les pointeurs modifiés en RAM très rapidement
    for (auto* v : m_voitures_flotte) {
        if (v) {
            // Utilise la requête préparée synchrone compilée dans ta DAL
            m_dalVoiture->mettre_a_jour_voiture(*v);
        }
    }

    // 3. Écriture physique globale sur le disque dur Ubuntu
    m_dbManager->valider_transaction();

    std::cout << "[Write-Behind] Flush synchrone effectué avec succès au Tick T = " 
              << m_temps_continue << " min (Fréquence Planificateur)." << std::endl;
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
void Simulateur::tick(int T) 
{
    m_temps_continue = T;

    // L'architecture repose sur le fait que le Planificateur fournit un accès modifiable
    // à ses listes pour permettre au Simulateur de mettre à jour l'état physique des convois.
    std::vector<Convoi>& convois_entree = m_planificateur.get_convois_entree();
    std::vector<Convoi>& convois_sortie = m_planificateur.get_convois_sortie();

    // ------------------------------------------------------------------------
    // [1] DÉPARTS DE PROVINCE (Anticipation des retours vers la capitale)
    // ------------------------------------------------------------------------
    for (auto& convoi : convois_entree) {
        if (convoi.get_etat() == EtatConvoi::PRET && convoi.get_taille() > 0)  // convoie pret 
        {
            
            // ÉTAPE 2 : On lit la région depuis le convoi, et non pas depuis la voiture
            int id_prov = convoi.get_id_region(); // c'est qui le setters , fait
            int duree_trajet = m_durees_trajet.at(id_prov);
            
            // Calcul de l'heure exacte où le convoi doit démarrer pour arriver à l'heure prévue au portail
            int t_depart = convoi.get_horaire_prevue() - duree_trajet;
            
            if (T >= t_depart) 
            {
                for (auto* v : convoi.get_voitures()) {
                    if (v) 
                    {
                        v->set_etat(EtatVoiture::EN_ROUTE);
                        
                        // ÉTAPE 3 : On force la destination vers la capitale (ID 0)
                        v->set_destination(m_origine); 
                        
                        v->set_heure_arrivee(static_cast<double>(convoi.get_horaire_prevue()));
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
        if (v && v->get_etat() == EtatVoiture::EN_ROUTE && v->get_destination() != m_origine) // 0 = Capitale
        { 
            if (T >= v->get_heure_arrivee()) 
            {
                int id_province = v->get_destination();
                
                // Injection des passagers dans le séjour de la province
                m_generateur.enregistrer_arrivee_province(id_province, v->get_passagers(), static_cast<double>(T));
                
                v->debarquer_tous();
                v->set_etat(EtatVoiture::EN_ATTENTE_STATION);
            }
        }
    }

    // ------------------------------------------------------------------------
    // [3] ARRIVÉES À LA GARE PRINCIPALE (Le Portail des Entrées - Anti-Embouteillage)
    // ------------------------------------------------------------------------
    for (auto& convoi : convois_entree) {
        if (convoi.get_etat() == EtatConvoi::EN_TRANSIT) {
            if (T >= convoi.get_horaire_prevue()) // il est arriver
            {
                
                // 1 & 2. Force le passage et verrouille le portail en cumulant le temps
                int duree_franchissement = convoi.get_taille() * m_duree_franchissement_par_voiture;
                m_portail_occupe_jusqua = std::max(m_portail_occupe_jusqua, T) + duree_franchissement;

                // 3. Sauvegarde car la libération vide la liste interne du convoi
                std::vector<Voiture*> voitures_arrivantes = convoi.get_voitures();

                // 4. Libère le convoi (passe les voitures EN_ATTENTE_GARE et le convoi TERMINE)
                convoi.liberer_voitures(-1.0);

                // 5. Vide les passagers (fin du voyage) et synchronise
                for (auto* v : voitures_arrivantes) {
                    if (v) 
                    {
                        v->debarquer_tous();
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // [4] FLUX DE PASSAGERS (Générateur & Billetterie) pourquoi ici ??
    // ------------------------------------------------------------------------
    m_generateur.generer_flux(static_cast<double>(T), m_billetterie); // appele billeterie::ajouter_reservation -> remplie m_carnet_reservations

    // ------------------------------------------------------------------------
    // [5] LE CERVEAU (Activation Cyclique du Planificateur)
    // ------------------------------------------------------------------------
    if (T % m_frequence_planif == 0)
    {        
        std::vector<Voiture*> voitures_gare;
        std::unordered_map<int, std::vector<Voiture*>> map_provinces;

        // Préparation du model de la flotte pour le Planificateur
        for (auto* v : m_voitures_flotte) 
        {
            if (v) 
            {
                if (v->get_etat() == EtatVoiture::EN_ATTENTE_GARE) 
                {
                    voitures_gare.push_back(v);
                } 
                else if (v->get_etat() == EtatVoiture::EN_ATTENTE_STATION) 
                {
                    map_provinces[v->get_destination()].push_back(v);
                }
            }
        }

        std::unordered_map<int, int> dep_std, dep_urg, ret_std, ret_urg;
        m_billetterie.extraire_demandes(static_cast<double>(T), dep_std, dep_urg, ret_std, ret_urg); // mets a jour le liste d'attente 

        // Planification (inclut l'auto-nettoyage des convois TERMINE)
        m_planificateur.planifier_global(dep_std, dep_urg, ret_std, ret_urg, voitures_gare, map_provinces, static_cast<double>(T));

        // Calcul et rapatriement des résidus vers la billetterie
        auto residus = m_planificateur.calculer_demande_residuelle(dep_std, dep_urg, ret_std, ret_urg, convois_sortie, convois_entree);
        
        m_billetterie.traiter_demande_residuelle(static_cast<double>(T), residus.first, residus.second);

        //Synchronisation ici : On profite du fait que le planificateur s'active pour synchroniser l'état
        synchroniser_bdd();

    }

    // ------------------------------------------------------------------------
    // [6] DÉPARTS DE LA GARE PRINCIPALE (Le Portail des Sorties)
    // ------------------------------------------------------------------------
    if (m_portail_occupe_jusqua > T || en_plage_interdite(T)) 
    {
        // Le portail est physiquement obstrué ou interdit, on ne fait rien.
    } 
    else 
    {
        Convoi* meilleur_candidat = nullptr;

        // Recherche du meilleur convoi de SORTIE prêt à partir :Urgent > Std
        for (auto& convoi : convois_sortie) {
            if (convoi.get_etat() == EtatConvoi::PRET && convoi.get_horaire_prevue() <= T) 
            {
                if (!meilleur_candidat) 
                {
                    meilleur_candidat = &convoi;
                } else {
                    // Priorité 1 : Urgence
                    if (convoi.contient_urgence() && !meilleur_candidat->contient_urgence()) // le convoie est urgent que meilleur_candidat
                    {
                        meilleur_candidat = &convoi;
                    } 
                    // Priorité 2 : Ancienneté d'horaire prévu
                    else if (convoi.contient_urgence() == meilleur_candidat->contient_urgence()) 
                    {
                        if (convoi.get_horaire_prevue() < meilleur_candidat->get_horaire_prevue()) 
                        {
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
        }
    }

    // [7] FIN DU TICK
}