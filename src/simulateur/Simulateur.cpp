#include "Simulateur.h"
#include "Configuration.h" // Nécessaire pour le parsing des CSV à l'étape 2
#include <filesystem>
#include <iostream>

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
                       DatabaseManager* dbManager,
                       DalVoiture* dalVoiture,
                       DalConvoi* dalConvoi,
                       DalClient* dalClient,
                       DalBillet* dalBillet
                    )
    : m_origine(id_origine)
    , m_temps_continue(0)
    , m_portail_occupe_jusqua(0)
    , m_duree_franchissement_par_voiture(duree_franchissement)
    // AJOUT : Garde-fou 
    , m_frequence_planif(frequence_planif > 0 ? frequence_planif : 15)
    , m_voitures_flotte(flotte_globale)
    , m_plages_interdites(plages)
    , m_durees_trajet(durees_trajet)
    , m_billetterie(billetterie)
    , m_generateur(generateur)
    , m_planificateur(planificateur)
    , m_dbManager(dbManager)  
    , m_dalVoiture(dalVoiture) 
    , m_dalConvoi(dalConvoi)
    , m_dalClient(dalClient)
    , m_dalBillet(dalBillet)
    , m_prochain_id_client(1) // 1. afak atao INITIALISATION PAR DÉFAUT ICI
{
    // REPRISE DES IDENTIFIANTS DES CONVOIS
    if (m_dalConvoi) {
        int dernier_id_convoi = m_dalConvoi->get_max_id_convoi();
        int prochain_id = dernier_id_convoi + 1;
        
        m_planificateur.set_prochain_id_convoi(prochain_id);
        
        std::cout << "[Simulateur] Initialisation ID Convoi : Le planificateur reprendra à l'ID " 
                  << prochain_id << " (Dernier archivé : " << dernier_id_convoi << ")." << std::endl;
    }

    // 2. REPRISE DES IDENTIFIANTS DES CLIENTS
    if (m_dalClient) {
        int dernier_id_client = m_dalClient->get_max_id_client();
        m_prochain_id_client = dernier_id_client + 1;
        
        std::cout << "[Simulateur] Initialisation ID Client : Le générateur reprendra à l'ID " 
                  << m_prochain_id_client << " (Dernier identifié : " << dernier_id_client << ")." << std::endl;
    }
}

// ============================================================================
// PERSISTANCE & VÉRIFICATIONS PHYSIQUES
// ============================================================================

// ============================================================================
// ÉTAPES 2 & 3 : ORCHESTRATION DU DÉMARRAGE (Méthode Statique)
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
    // FIX SÉCURITÉ : Au lieu de se fier à la taille du fichier (trompeur avec SQLite),
    // on force le premier lancement si on est dans un contexte de TEST ou si le fichier n'existait pas au tout début.
    // Pour le test, on va simplement vérifier si la table essentielle "dal_voitures" est initialisée et contient des données plus tard,
    // ou utiliser un booléen clair. Ici, on va forcer la détection brute :
    
    if (!db.initialiser()) {
        std::cerr << "[Orchestrateur] ERREUR : Impossible d'initialiser SQLite." << std::endl;
        return false;
    }
    
    DalDestination dalDest(db.get_connexion());
    DalCooperative dalCoop(db.get_connexion());
    DalPlageInterdite dalPlage(db.get_connexion());
    DalConfiguration dalConfig(db.get_connexion());
    
    int t_charge = 0;
    int t_decharge = 0;

    // Compter le nombre de voitures en base pour savoir si on doit amorcer
    //  RECTIFICATION D'UN BUG : On vérifie d'abord si la table existe avant de la lire
    bool base_vide = true;
    if (db.table_existe("dal_voitures")) {
        DalVoiture dalVoitureVerif(db.get_connexion(), 0, 0);
        base_vide = dalVoitureVerif.charger_tout().empty();
    }

    if (base_vide) {
        std::cout << "[Orchestrateur] Base vide détectée. Exécution du schéma et chargement des CSV..." << std::endl;
        
        if (!db.executer_script_sql("data/-- SQLite.sql")) {
            std::cerr << "ERREUR FATALE : Le script SQL est introuvable ou invalide." << std::endl;
            return false;
        }

        Configuration config;
        config.charger("requirement"); // Chargement des CSV

        //  SECURITÉ INTERDICTION DE CONTINUER SI LES CSV SONT VIDES
        if (config.get_voitures().empty()) {
            std::cerr << "\n[ERREUR CRITIQUE] config.get_voitures() est VIDE !" << std::endl;
            std::cerr << "[REMEDE] Le dossier 'requirement/' est introuvable depuis l'emplacement d'exécution du test." << std::endl;
            std::cerr << "[REMEDE] Emplacement actuel : " << std::filesystem::current_path() << std::endl;
            return false; // Arrêt propre de l'orchestrateur
        }

        t_charge = config.get_parametre("temps_chargement");
        t_decharge = config.get_parametre("temps_dechargement");

        DalVoiture dalVoiture(db.get_connexion(), t_charge, t_decharge);
        
        db.commencer_transaction();
        
        // 1. ON INSÈRE D'ABORD LES RÉFÉRENTIELS (Pas de dépendances)
        for (const auto& paire : config.get_destinations()) {
            dalDest.inserer_destination(paire.second);
            std::cout << "Destination lue - ID: " << paire.second.get_id() << " | Nom: " << paire.second.get_nom() << std::endl;

        }
        auto verif_dest = dalDest.charger_tout();
        std::cout << "[DEBUG] Nombre de destinations chargées : " << verif_dest.size() << std::endl;
        for(const auto& d : verif_dest) {
            if(d.get_id() == 0) std::cout << "[DEBUG] Destination 0 trouvée : " << d.get_nom() << std::endl;
        }

        for (const auto& paire : config.get_cooperatives()) {
            dalCoop.inserer_cooperative(paire.second);
        }

        for (const auto& plage : config.get_plages()) {
            dalPlage.inserer_plage(plage);
        }

        for (const auto& [cle, val] : config.get_parametres()) {
            dalConfig.sauvegarder_parametre(cle, val);
        }

        // 2. ENFIN, ON INSÈRE LES VOITURES (Leurs clés étrangères pointent maintenant vers des données réelles !)
        int compteur_insertions = 0;
        for (const auto& paire : config.get_voitures()) {
            if (dalVoiture.mettre_a_jour_voiture(paire.second)) {
                compteur_insertions++;
            }
        }
        std::cout << "[Orchestrateur] " << compteur_insertions << " voitures insérées en BDD via UPSERT." << std::endl;
        // Ajoute ce bloc dans l'orchestrateur juste après l'insertion des destinations
        db.valider_transaction();

    } else {
        std::cout << "[Orchestrateur] La base contient déjà des données. Passage au chargement RAM direct." << std::endl;
        std::unordered_map<std::string, int> params_temporaires = dalConfig.charger_parametres();
        t_charge = params_temporaires["temps_chargement"];
        t_decharge = params_temporaires["temps_dechargement"];
    }
    
    // --- ÉTAPE 3 : CHARGEMENT MASSIF EN RAM ---
    DalVoiture dalVoitureFinal(db.get_connexion(), t_charge, t_decharge);

    conteneur_physique = dalVoitureFinal.charger_tout();
    destinations_ram = dalDest.charger_tout();
    cooperatives_ram = dalCoop.charger_tout();
    plages_ram = dalPlage.charger_tout();
    parametres_ram = dalConfig.charger_parametres();

    flotte_pointeurs.clear();
    for (auto& voiture : conteneur_physique) {
        flotte_pointeurs.push_back(&voiture); 
    }

    std::cout << "[Orchestrateur] Fin du chargement. Voitures chargées en RAM : " << flotte_pointeurs.size() << std::endl;
    
    // Si malgré tout la flotte est vide ici, on refuse de dire que le démarrage est un succès
    return !flotte_pointeurs.empty();
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

bool Simulateur::en_plage_interdite(int temps) const noexcept 
{
    for (const auto& plage : m_plages_interdites) 
    {
        // La plage s'occupe elle-même de gérer le format circulaire et le passage à minuit
        if (plage.contient(temps)) {
            return true;
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
    m_generateur.generer_flux(m_temps_continue, m_billetterie, m_dalClient, m_prochain_id_client); // appele billeterie::ajouter_reservation -> remplie m_carnet_reservations
    
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

            for (auto* v : voitures_partantes) {
                if (v) {
                    v->set_etat(EtatVoiture::EN_ROUTE);
                    v->set_heure_arrivee(heure_arrivee_province);
                    // CORRECTION CRITIQUE : Fixation de la boussole physique
                    v->set_destination(id_dest); 
                }
            }
            meilleur_candidat->set_etat(EtatConvoi::EN_TRANSIT);
        }
    }

    // [7] FIN DU TICK
}

/*
Si tu as 5 000 clients en attente dans toute la gare, tu vas instancier 5 000 objets, itérer dessus pour en trouver 4, puis jeter les 4 996 autres.
C'est une fuite de performance massive (fuite de temps CPU et de RAM) qui va figer ton simulateur à mesure que la journée avance.
La solution : Déléguer ce travail à SQLite. Crée une méthode spécifique dans DalClient qui ne remonte que les clients nécessaires, avec une limite stricte.
*/
// pour gerer la base de donner
void Simulateur::enregistrer_embarquement(int id_voiture, int id_destination, int nb_passagers_a_embarquer, double prix_du_billet) 
{
    if (!m_dalClient || !m_dalBillet || !m_dbManager) return; 

    // 1. Délégation à SQLite : On récupère uniquement les N passagers prioritaires
    std::vector<Client> clients_a_embarquer = m_dalClient->extraire_clients_pour_embarquement(id_destination, nb_passagers_a_embarquer);

    // 2. Optimisation I/O : On ouvre une transaction pour l'embarquement
    m_dbManager->commencer_transaction();

    int embarques = 0;
    for (const auto& client : clients_a_embarquer) {
        
        // Création du Billet
        Billet nouveau_billet(
            client.get_id(), 
            id_voiture, 
            client.get_t_min(), 
            client.get_t_max(), 
            prix_du_billet
        );

        // Insertion et Suppression par lots dans la transaction
        if (m_dalBillet->inserer_billet(nouveau_billet)) {
            m_dalClient->supprimer_client(client.get_id());
            embarques++;
        }
    }

    // 3. Validation de l'écriture de masse
    m_dbManager->valider_transaction();

    // ---------------------------------------------------------
    // 4. CORRECTION : MISE À JOUR DE LA MÉMOIRE RAM VIA EMBARQUER
    // ---------------------------------------------------------
    if (embarques > 0) {
        for (Voiture* v : m_voitures_flotte) {
            if (v->get_id() == id_voiture) {
                // On utilise ta méthode métier existante
                bool succes = v->embarquer(embarques);
                
                //On met à jour la cible physique de la voiture !
                v->set_destination(id_destination);

                if (!succes) {
                    std::cerr << "[ERREUR SIMULATEUR] Impossible d'embarquer " << embarques 
                              << " passagers dans la voiture #" << id_voiture 
                              << " (Places libres : " << v->get_places_libres() << ")" << std::endl;
                }
                break;
            }
        }
    }
}