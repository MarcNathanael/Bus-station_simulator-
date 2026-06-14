#include "Simulateur.h"
#include "Configuration.h" // Nécessaire pour le parsing des CSV à l'étape 2
#include <filesystem>
#include <iostream>

// ============================================================================
// CONSTRUCTEU
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
                       DalVoiture* dalVoiture,
                       DalConvoi* dalConvoi
                    )
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
    , m_dalConvoi(dalConvoi)
{
    // REPRISE DES IDENTIFIANTS
    if (m_dalConvoi) {
        int dernier_id_convoi = m_dalConvoi->get_max_id_convoi();
        int prochain_id = dernier_id_convoi + 1;
        
        m_planificateur.set_prochain_id_convoi(prochain_id);
        
        std::cout << "[Simulateur] Initialisation ID Convoi : Le planificateur reprendra à l'ID " 
                  << prochain_id << " (Dernier archivé : " << dernier_id_convoi << ")." << std::endl;
    }
}

// ============================================================================
// PERSISTANCE & VÉRIFICATIONS PHYSIQUES
// ============================================================================

// ============================================================================
// ÉTAPES 2 & 3 : ORCHESTRATION DU DÉMARRAGE (Méthode Statique)
// ============================================================================
bool Simulateur::orchestrer_demarrage(
    DatabaseManager& db, 
    std::vector<Voiture>& conteneur_physique, 
    std::vector<Voiture*>& flotte_pointeurs,
    std::vector<Destination>& destinations_ram,
    std::vector<Cooperative>& cooperatives_ram,
    std::vector<PlageInterdite>& plages_ram,
    std::unordered_map<std::string, int>& parametres_ram) 
{
    const std::string chemin_db = "data/db.sqlite";
    bool premier_lancement = !std::filesystem::exists(chemin_db) || std::filesystem::is_empty(chemin_db);

    if (!db.initialiser()) {
        std::cerr << "[Orchestrateur] ERREUR : Impossible d'initialiser SQLite." << std::endl;
        return false;
    }
    
    // 1. Instanciation de TOUTES les DALs en lecture/écriture ponctuelle
    DalVoiture dalVoiture(db.get_connexion());
    DalDestination dalDest(db.get_connexion());
    DalCooperative dalCoop(db.get_connexion());
    DalPlageInterdite dalPlage(db.get_connexion());
    DalConfiguration dalConfig(db.get_connexion());
    
    // --- ÉTAPE 2 : AMORÇAGE AU PREMIER LANCEMENT (CSV -> SQLite) ---
    if (premier_lancement) {
        std::cout << "[Orchestrateur] Premier lancement. Exécution du schéma..." << std::endl;
        db.executer_script_sql("data/-- SQLite.sql");

        Configuration config;
        config.charger("requirement"); // Chargement des CSV

        db.commencer_transaction();
        
        // Déversement global des CSV vers SQLite
        for (const auto& paire : config.get_voitures()) dalVoiture.inserer_voiture(paire.second);
        
        // NOUVEAU : Insertion des autres entités (en supposant que config possède ces getters)

        // 'paire' contient {key, value}
        for (const auto& paire : config.get_destinations()) {
            dalDest.inserer_destination(paire.second); // On n'envoie que la Destination
        }

        for (const auto& paire : config.get_cooperatives()) {
            dalCoop.inserer_cooperative(paire.second);
        }
        for (const auto& paire : config.get_cooperatives()){ 
            dalCoop.inserer_cooperative(paire.second);
        }
        for (const auto& plage : config.get_plages()) dalPlage.inserer_plage(plage);
        for (const auto& [cle, val] : config.get_parametres()) dalConfig.sauvegarder_parametre(cle, val);
        
        db.valider_transaction();
        std::cout << "[Orchestrateur] Base SQLite initialisée avec succès." << std::endl;
    }
    
    // --- ÉTAPE 3 : CHARGEMENT MASSIF EN RAM SQLite -> RAM ---
    std::cout << "[Orchestrateur] Chargement du référentiel en RAM..." << std::endl;
    
    conteneur_physique = dalVoiture.charger_tout();
    destinations_ram = dalDest.charger_tout();
    cooperatives_ram = dalCoop.charger_tout();
    plages_ram = dalPlage.charger_tout();
    parametres_ram = dalConfig.charger_parametres();

    // Population des pointeurs de voitures
    flotte_pointeurs.clear();
    for (auto& voiture : conteneur_physique) {
        flotte_pointeurs.push_back(&voiture); 
    }

    std::cout << "[Orchestrateur] Chargement terminé." << std::endl;
    return true;
}
// ============================================================================
// ÉTAPE 4 : ÉCRITURE DIFFÉRÉE (WRITE-BEHIND PAR LOTS)
// ============================================================================
// RAM -> SQL
void Simulateur::synchroniser_bdd() 
{
    if (!m_dbManager || !m_dalVoiture || !m_dalConvoi) return;

    m_dbManager->commencer_transaction();

    int compte_voitures = 0;
    int compte_convois = 0;

    // 1. Synchronisation de la flotte de voitures (Toujours active à chaque cycle)
    for (auto* v : m_voitures_flotte) {
        if (v && v->is_dirty()) { 
            m_dalVoiture->mettre_a_jour_voiture(*v);
            v->clear_dirty();
            compte_voitures++;
        }
    }

    // Fonction lambda locale pour traiter les deux listes du Planificateur
    auto archiver_convois_termines = [&](std::vector<Convoi>& liste_convois) {
        for (auto& convoi : liste_convois) {
            // On n'archive QUE si le convoi a changé ET qu'il a fini sa mission
            if (convoi.is_dirty() && convoi.get_etat() == EtatConvoi::TERMINE) {
                if (m_dalConvoi->archiver_convoi(convoi)) {
                    convoi.clear_dirty(); // On enlève le flag pour ne pas l'archiver deux fois
                    compte_convois++;
                }
            }
        }
    };

    // 2. Scan et archivage des convois d'entrée et de sortie
    archiver_convois_termines(m_planificateur.get_convois_entree());
    archiver_convois_termines(m_planificateur.get_convois_sortie());

    m_dbManager->valider_transaction();

    if (compte_voitures > 0 || compte_convois > 0) {
        std::cout << "[Write-Behind] Synchro BDD : " << compte_voitures << " voitures mises à jour, "
                  << compte_convois << " convois archivés à T = " << m_temps_continue << " min." << std::endl;
    }
}

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

        m_planificateur.nettoyer_convois_passes(static_cast<double>(T));
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