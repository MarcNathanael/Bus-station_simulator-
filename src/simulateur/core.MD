Mon projet consiste a gerer l'entre-sortie dans une gare routiere de facon a respecter plusieur contraintes.

Objectif  : eviter le chevauchement et les bouchons au niveau du portail de la gare principale tout en minimisant le temps d'attente des clients et maximisant le rendement des cooperatives 

Tout les prerequie seront charger a partir de fichier csv , voici les classe principale :

Voiture.h :

enum class EtatVoiture {
    EN_ATTENTE_GARE,
    EN_CHARGEMENT,
    EN_ROUTE,
    EN_ATTENTE_STATION,
    EN_ROUTE_RETOUR
};

// Fonction utilitaire pour parser l'état depuis un CSV
// inline exige que le coprs de la fonction soit visible dans le hearders qui l'inclut , pas de prototype
inline EtatVoiture stringToEtatVoiture(const std::string& s)
{
    if (s == "EN_ATTENTE_GARE") return EtatVoiture::EN_ATTENTE_GARE;
    if (s == "EN_CHARGEMENT") return EtatVoiture::EN_CHARGEMENT;
    if (s == "EN_ROUTE") return EtatVoiture::EN_ROUTE;
    if (s == "EN_ATTENTE_STATION") return EtatVoiture::EN_ATTENTE_STATION;
    if (s == "EN_ROUTE_RETOUR") return EtatVoiture::EN_ROUTE_RETOUR;
    throw std::runtime_error("EtatVoiture inconnu : " + s);
}

class Voiture {
    public:
        // Constructeur simple (utilisé par défaut dans les tests)
        Voiture(int id, int id_coop, int id_destination);

        // Constructeur complet (utilisé par Configuration)
        Voiture(int id, int id_coop, int id_destination, int id_position,
                int capacite_max, int places_libres, EtatVoiture etat, int horaire_depart);

        // Getters
        int get_id() const;
        int get_id_coop() const;
        int get_places_libres() const;
        int get_places_max() const;
        int get_horaire_depart() const;
        EtatVoiture get_etat() const;
        int get_position() const;
        int get_destination() const;

        // Actions métier
        bool embarquer(int nb_passagers);
        bool est_pleine() const;
        void Voiture::debarquer(int nb);


        // Setters
        void set_etat(EtatVoiture etat);
        void set_position(int pos);
        void set_horaire_depart(int minutes);
        void set_destination(int id_dest);
    private:
        friend class Cooperative;

        int m_id;
        int m_id_coop;
        int m_nb_places_libres;
        int m_nb_places_max;
        int m_horaire_depart;
        EtatVoiture m_etat;
        int m_id_position;
        int m_id_destination;
        Voiture* m_suivant;
};

#include "Voiture.h"

// ─── Constructeur simple ──────────────────────────────────
Voiture::Voiture(int id, int id_coop, int id_destination)
    : Voiture(id, id_coop, id_destination,
        0, // position = GARE_PRINCIPAL (ID 0)
        32, // capacité max
        32, // places libres
        EtatVoiture::EN_ATTENTE_GARE,
        -1) // horaire non planifié
{
    // Le corps est vide
}

// ─── Constructeur complet ─────────────────────────────────
Voiture::Voiture(int id, int id_coop, int id_destination, int id_position,
                 int capacite_max, int places_libres, EtatVoiture etat, int horaire_depart)
    : m_id(id),
    m_id_coop(id_coop),
    m_nb_places_libres(places_libres),
    m_nb_places_max(capacite_max),
    m_horaire_depart(horaire_depart),
    m_etat(etat),
    m_id_position(id_position),
    m_id_destination(id_destination),
    m_suivant(nullptr)
{
}

// ─── Getters ──────────────────────────────────────────────
int Voiture::get_id() const { return m_id; }
int Voiture::get_id_coop() const { return m_id_coop; }
int Voiture::get_places_libres() const { return m_nb_places_libres; }
int Voiture::get_places_max() const { return m_nb_places_max; }
int Voiture::get_horaire_depart() const { return m_horaire_depart; }
EtatVoiture Voiture::get_etat() const { return m_etat; }
int Voiture::get_position() const { return m_id_position; }
int Voiture::get_destination() const { return m_id_destination; }

// ─── Actions métier ──────────────────────────────────────
bool Voiture::embarquer(int nb_passagers) 
{
    if (nb_passagers <= 0) return false;
    if (m_nb_places_libres >= nb_passagers) 
    {
        m_nb_places_libres -= nb_passagers;
        return true;
    }
    return false;
}

void Voiture::debarquer(int nb) {
    m_nb_places_libres += nb;
    // sans dépasser la capacité max
    if (m_nb_places_libres > m_nb_places_max) 
    {
        m_nb_places_libres = m_nb_places_max;
    }
}

bool Voiture::est_pleine() const 
{
    return m_nb_places_libres == 0;
}

// ─── Setters ─────────────────────────────────────────────
void Voiture::set_etat(EtatVoiture etat) { m_etat = etat; }
void Voiture::set_position(int pos) { m_id_position = pos; }
void Voiture::set_horaire_depart(int minutes) { m_horaire_depart = minutes; }
void Voiture::set_destination(int id_dest) { m_id_destination = id_dest; }


Destination :

class Destination {
public:
    Destination(int id, const std::string& nom, int duree_trajet);

    int get_id() const;
    std::string get_nom() const;
    int get_duree_trajet() const;

private:
    int m_id;
    std::string m_nom;
    int m_duree_trajet;   // en minutes
};


Destination::Destination(int id, const std::string& nom, int duree_trajet)
    : m_id(id), 
      m_nom(nom),
      m_duree_trajet(duree_trajet)
{
}

int Destination::get_id() const 
{ 
    return m_id; 
}
std::string Destination::get_nom() const 
{ 
    return m_nom; 
}

int Destination::get_duree_trajet() const 
{ 
    return m_duree_trajet; 
}

Cooperative :
class Cooperative {
public:
    Cooperative(int id, const std::string& nom);

    int get_id() const;
    std::string get_nom() const;

    // Gestion de la flotte
    void ajouter_voiture(Voiture* v);
    void retirer_voiture(int id_voiture);
    const std::vector<Voiture*>& get_voitures() const;
    int get_nb_voitures() const;

    // Recherche métier
    Voiture* trouver_voiture_disponible(int id_destination) const;
    // Retourne une voiture EN_ATTENTE_GARE, avec places libres, 
    // assignée à cette destination. nullptr si aucune.

private:
    int m_id;
    std::string m_nom;
    std::vector<Voiture*> m_voitures; // evite la copie
};


Cooperative::Cooperative(int id, const std::string& nom)
    : m_id(id), m_nom(nom)
{
}

int Cooperative::get_id() const { return m_id; }
std::string Cooperative::get_nom() const { return m_nom; }

// ─── GESTION DE LA FLOTTE ─────────────────────────────────
void Cooperative::ajouter_voiture(Voiture* v) {
    if (v) {
        m_voitures.push_back(v);
    }
}

void Cooperative::retirer_voiture(int id_voiture) {
    for (auto it = m_voitures.begin(); it != m_voitures.end(); ++it) 
    {
        if ((*it)->get_id() == id_voiture) 
        {
            m_voitures.erase(it);
            return;
        }
    }
}

const std::vector<Voiture*>& Cooperative::get_voitures() const {
    return m_voitures;
}

int Cooperative::get_nb_voitures() const {
    return static_cast<int>(m_voitures.size());
}

// ─── RECHERCHE MÉTIER ─────────────────────────────────────
Voiture* Cooperative::trouver_voiture_disponible(int id_destination) const {
    for (auto* v : m_voitures) 
    {
        if (v->get_etat() == EtatVoiture::EN_ATTENTE_GARE          // à la gare
            && v->get_destination() == id_destination               // bonne destination
            && !(v->est_pleine()) ) {                                  // places libres
            return v; // la vraie voiture pas une copie
        }
    }
    return nullptr;  // aucune voiture disponible grace au pointeur 
}


Convoi :

enum class TypeConvoi {
    SORTIE,
    ENTREE
};

enum class EtatConvoi {
    EN_FORMATION,      // en attente de remplissage
    PRET,              // complet ou fermé, en attente de passage
    EN_FRANCHISSEMENT, // en train de passer le portail
    TERMINE            // passé, voitures libérées
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

        // Gestion des voitures
        bool ajouter_voiture(Voiture* v);
        bool retirer_voiture(int id_voiture);
        bool est_plein() const;

        // Changement d'état
        void set_etat(EtatConvoi etat);
        void set_horaire_prevue(int minutes);

        // Libère toutes les voitures (les retire du convoi)
        void liberer_voitures();

    private:
        bool Convoi::voiture_est_disponible(const Voiture* v) const;

        int m_id;
        TypeConvoi m_type;
        EtatConvoi m_etat;
        int m_horaire_prevue;
        std::vector<Voiture*> m_voitures; // evite la copie
        static const int TAILLE_MAX = 8;
};


Convoi::Convoi(int id, TypeConvoi type)
    : m_id(id), m_type(type), m_etat(EtatConvoi::EN_FORMATION), m_horaire_prevue(-1)
{
}

int Convoi::get_id() const { return m_id; }
TypeConvoi Convoi::get_type() const { return m_type; }
EtatConvoi Convoi::get_etat() const { return m_etat; }
int Convoi::get_taille() const { return static_cast<int>(m_voitures.size()); }
int Convoi::get_taille_max() const { return TAILLE_MAX; }
int Convoi::get_horaire_prevue() const { return m_horaire_prevue; }
const std::vector<Voiture*>& Convoi::get_voitures() const { return m_voitures; }

bool Convoi::voiture_est_disponible(const Voiture* v) const // ← vérification
{
    if (!v) return false;
    
    if (m_type == TypeConvoi::SORTIE) {
        // Pour un convoi sortant, la voiture doit être en attente à la gare
        return v->get_etat() == EtatVoiture::EN_ATTENTE_GARE;
    } else {
        // Pour un convoi entrant, la voiture doit être en attente à la station
        return v->get_etat() == EtatVoiture::EN_ATTENTE_STATION;
    }
}

bool Convoi::ajouter_voiture(Voiture* v) {
    if (!v) return false;

    // 2. Le convoi doit être en cours de formation et non plein
    if (m_etat != EtatConvoi::EN_FORMATION) return false; // ne peut ajouter qu'en formation
    if (est_plein()) return false;

    // 3. Verification
    if (!voiture_est_disponible(v)) return false;  // ← vérification

    // 4. Validation et transition d'état de la voiture
    v->set_etat(EtatVoiture::EN_CHARGEMENT);

    m_voitures.push_back(v);

    return true;
}

// ─── RETIRER UNE VOITURE (modifié) ────────────────────────
bool Convoi::retirer_voiture(int id_voiture) {
    for (auto it = m_voitures.begin(); it != m_voitures.end(); ++it) {
        if ((*it)->get_id() == id_voiture) {
            // Remet la voiture dans son état d'attente
            if (m_type == TypeConvoi::SORTIE) {
                (*it)->set_etat(EtatVoiture::EN_ATTENTE_GARE);
            } else {
                (*it)->set_etat(EtatVoiture::EN_ATTENTE_STATION);
            }
            m_voitures.erase(it);
            return true;
        }
    }
    return false;
}

bool Convoi::est_plein() const {
    return m_voitures.size() >= TAILLE_MAX;
}

void Convoi::set_etat(EtatConvoi etat) {
    m_etat = etat;
}

void Convoi::set_horaire_prevue(int minutes) {
    m_horaire_prevue = minutes;
}

void Convoi::liberer_voitures() {
    for (auto* v : m_voitures) {
        if (m_type == TypeConvoi::SORTIE) {
            v->set_etat(EtatVoiture::EN_ROUTE);// part vers la destination
        } else {
            v->set_etat(EtatVoiture::EN_ATTENTE_GARE); // revenu à la gare
        }
    }
    m_voitures.clear();
    m_etat = EtatConvoi::TERMINE;
}


Plageinterdite :

class PlageInterdite {
public:
    PlageInterdite(int debut, int fin);

    int get_debut() const;
    int get_fin() const;
    int get_duree() const;                    // durée en minutes

    bool contient(int horaire) const;          // l'horaire est-il dans la plage ?
    bool est_trop_proche(int horaire, int marge) const; // l'horaire est-il à moins de 'marge' minutes ?

private:
    int m_debut;  // minutes depuis minuit
    int m_fin;    // minutes depuis minuit
};


PlageInterdite::PlageInterdite(int debut, int fin)
    : m_debut(debut), m_fin(fin)
{
    // variant :
    if (debut < 0 || fin < 0 || debut >= fin) 
    {
        throw std::invalid_argument(
            "Plage invalide : debut=" + std::to_string(debut) 
            + " fin=" + std::to_string(fin)
        );
    }
}

int PlageInterdite::get_debut() const { return m_debut; }
int PlageInterdite::get_fin() const   { return m_fin; }

int PlageInterdite::get_duree() const {
    return m_fin - m_debut;
}

// ─── VÉRIFICATIONS ────────────────────────────────────────

bool PlageInterdite::contient(int horaire) const {
    // Strictement à l'intérieur (les bornes sont incluses ? 
    // On considère que le franchissement ne peut pas commencer 
    // exactement à la borne, donc on exclut les bornes)
    return horaire > m_debut && horaire < m_fin;
}

bool PlageInterdite::est_trop_proche(int horaire, int marge) const {
    // Vérifie si l'horaire est dans la zone dangereuse autour de la plage
    // Zone dangereuse = [debut - marge, fin + marge]
    return horaire >= (m_debut - marge) && horaire <= (m_fin + marge);
}


Configuration :

class Configuration {
public:
    // Charge tous les CSV depuis un dossier (ex: "data/")
    bool charger(const std::string& dossier);

    // Accès aux données chargées et les attribur sont imodifiable
    const std::unordered_map<int, Destination>& get_destinations() const;
    const std::unordered_map<int, Cooperative>& get_cooperatives() const;
    const std::unordered_map<int, Voiture>& get_voitures() const;
    const std::vector<PlageInterdite>& get_plages() const;
    int get_parametre(const std::string& cle) const;

private:
    // Fonctions de parsing individuelles
    void parser_destinations(const std::string& chemin);
    void parser_cooperatives(const std::string& chemin);
    void parser_voitures(const std::string& chemin);
    void parser_plages(const std::string& chemin);
    void parser_parametres(const std::string& chemin);

    std::unordered_map<int, Destination> m_destinations;
    std::unordered_map<int, Cooperative> m_cooperatives;
    std::unordered_map<int, Voiture> m_voitures;
    std::vector<PlageInterdite> m_plages;
    std::unordered_map<std::string, int> m_parametres;
};


bool Configuration::charger(const std::string& dossier) {
    try {
        parser_destinations(dossier + "/destinations.csv");
        parser_cooperatives(dossier + "/cooperatives.csv");
        parser_voitures(dossier + "/voitures.csv");
        parser_plages(dossier + "/plages_interdites.csv");
        parser_parametres(dossier + "/parametres.csv");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Erreur chargement configuration : " << e.what() << std::endl;
        return false;
    }
}

// ─── DESTINATIONS ────────────────────────────────────────
void Configuration::parser_destinations(const std::string& chemin) {
    std::ifstream fichier(chemin);
    if (!fichier) throw std::runtime_error("Impossible d'ouvrir " + chemin);
    std::string ligne;
    std::getline(fichier, ligne); // ignorer en-tête

    while (std::getline(fichier, ligne)) {
        if (ligne.empty()) continue;
        size_t v1 = ligne.find(',');
        size_t v2 = ligne.find(',', v1 + 1);
        int id = std::stoi(ligne.substr(0, v1));
        std::string nom = ligne.substr(v1 + 1, v2 - v1 - 1);
        int duree = std::stoi(ligne.substr(v2 + 1));
        m_destinations.emplace(id, Destination(id, nom, duree));
    }
}

// ─── COOPERATIVES ────────────────────────────────────────
void Configuration::parser_cooperatives(const std::string& chemin) {
    std::ifstream fichier(chemin);
    if (!fichier) throw std::runtime_error("Impossible d'ouvrir " + chemin);
    std::string ligne;
    std::getline(fichier, ligne);

    while (std::getline(fichier, ligne)) {
        if (ligne.empty()) continue;
        size_t v = ligne.find(',');
        int id = std::stoi(ligne.substr(0, v));
        std::string nom = ligne.substr(v + 1);
        m_cooperatives.emplace(id, Cooperative(id, nom));
    }
}

// ─── VOITURES ────────────────────────────────────────────
void Configuration::parser_voitures(const std::string& chemin) {
    std::ifstream fichier(chemin);
    if (!fichier) throw std::runtime_error("Impossible d'ouvrir " + chemin);
    std::string ligne;
    std::getline(fichier, ligne);

    while (std::getline(fichier, ligne)) {
        if (ligne.empty()) continue;
        // id,coop_id,destination_id,position_id,capacite_max,places_libres,etat,horaire_depart
        std::stringstream ss(ligne);
        std::string token;
        std::vector<std::string> champs;
        while (std::getline(ss, token, ',')) {
            champs.push_back(token);
        }

        int id = std::stoi(champs[0]);
        int coop_id = std::stoi(champs[1]);
        int dest_id = std::stoi(champs[2]);
        int pos_id = std::stoi(champs[3]);
        int cap_max = std::stoi(champs[4]);
        int places = std::stoi(champs[5]);
        EtatVoiture etat = stringToEtatVoiture(champs[6]);
        int horaire = std::stoi(champs[7]);

        // Construit la voiture avec les paramètres nécessaires
        // (On suppose un constructeur adapté, sinon on utilise les setters)
        Voiture v(id, coop_id, dest_id, pos_id, cap_max, places, etat, horaire);
        m_voitures.emplace(id, v);
    }
}

// ─── PLAGES INTERDITES ───────────────────────────────────
void Configuration::parser_plages(const std::string& chemin) {
    std::ifstream fichier(chemin);
    if (!fichier) throw std::runtime_error("Impossible d'ouvrir " + chemin);
    std::string ligne;
    std::getline(fichier, ligne);

    while (std::getline(fichier, ligne)) 
    {
        if (ligne.empty()) continue;
        size_t v = ligne.find(',');
        int debut = std::stoi(ligne.substr(0, v));
        int fin = std::stoi(ligne.substr(v + 1));
        m_plages.emplace_back(debut, fin);
    }
}

// ─── PARAMETRES ──────────────────────────────────────────
void Configuration::parser_parametres(const std::string& chemin) {
    std::ifstream fichier(chemin);
    if (!fichier) throw std::runtime_error("Impossible d'ouvrir " + chemin);
    std::string ligne;
    std::getline(fichier, ligne);

    while (std::getline(fichier, ligne)) {
        if (ligne.empty()) continue;
        size_t v = ligne.find(',');
        std::string cle = ligne.substr(0, v);
        int valeur = std::stoi(ligne.substr(v + 1));
        m_parametres[cle] = valeur;
    }
}

// ─── GETTERS ─────────────────────────────────────────────
const std::unordered_map<int, Destination>& Configuration::get_destinations() const { return m_destinations; }
const std::unordered_map<int, Cooperative>& Configuration::get_cooperatives() const { return m_cooperatives; }
const std::unordered_map<int, Voiture>& Configuration::get_voitures() const { return m_voitures; }
const std::vector<PlageInterdite>& Configuration::get_plages() const { return m_plages; }

int Configuration::get_parametre(const std::string& cle) const 
{
    auto it = m_parametres.find(cle);
    // si on a trouver une cle : sinon envoyer un throw (Sortie de secours, pas de return nécessaire)
    if (it != m_parametres.end()) return it->second;// c'est le parametre
    throw std::runtime_error("Paramètre inconnu : " + cle);
    //La fonction s'interrompt immédiatement.
    //Elle "lance" un objet d'erreur qui va remonter dans le programme jusqu'à trouver un bloc try { ... } catch { ... } pour gérer le problème.
}

Voici les csv :
cooperatives.csv :
id,nom
1,Coop Nord
2,Coop Sud
3,Coop Est

destination.csv:
id,nom,duree_minutes
0,GARE_PRINCIPAL,0
1,DIEGO,1440
2,MAJUNGA,900
3,TAMATAVE,420
4,AMBATONDRAZAKA,480
5,ANTSIRABE,210
6,FIANARANTSOA,420
7,TOLIARA,1200

parametres.csv:
cle,valeur
debut_journee,0
fin_journee,1440
espacement_min_entre_occupation_convois,15
capacite_defaut,32
marge_duree_trajet,30
nb_max_cooperatives,10
nb_max_voitures_par_coop,30
temps_chargement,15
temps_dechargement,10
duree_min_avant_annulation,15
duree_min_achat_avant_depart,15
taille_max_convoi,8
duree_franchissement_voiture,2
taux_remplissage_min,70
seuil_critique_suppression,20
poids_alpha,10
poids_beta,5
poids_gamma,1

plages_interdites.csv:
heure_debut,heure_fin
0,360
1200,1260

voiture.csv:
id,coop_id,destination_id,position_id,capacite_max,places_libres,etat,horaire_depart
1,1,1,0,32,32,EN_ATTENTE_GARE,-1
2,1,2,0,32,32,EN_ATTENTE_GARE,-1
3,2,3,0,32,32,EN_ATTENTE_GARE,-1
4,2,4,0,32,32,EN_ATTENTE_GARE,-1
5,3,5,0,32,32,EN_ATTENTE_GARE,-1
6,3,6,0,32,32,EN_ATTENTE_GARE,-1
7,1,7,0,32,32,EN_ATTENTE_GARE,-1
8,2,1,0,32,32,EN_ATTENTE_GARE,-1
9,1,0,1,32,32,EN_ATTENTE_STATION,-1
10,2,0,2,32,32,EN_ATTENTE_STATION,-1
11,3,0,3,32,32,EN_ATTENTE_STATION,-1
12,1,0,4,32,32,EN_ATTENTE_STATION,-1
13,2,0,5,32,32,EN_ATTENTE_STATION,-1
14,3,0,6,32,32,EN_ATTENTE_STATION,-1
15,1,0,7,32,32,EN_ATTENTE_STATION,-1


Voici la partie Simulateur :
Il faut bien comprendre les trois class majeur : Generateur -> Billeterie -> Planificateur

1)-Generateur :
Generer des clients qui reserve a l'avant (avant temps minimal d'achat ) , les clients donne un interval ou il veut partir : [t_min,t_max].
Utilise la loi de poisson non homogene pour generer un flux qui simule la realiter .
Controle le plafond des nombre de clients de la gare .
Il feras ca pour les depart province et retour province 
Pour rentdre le flux plus normal , le generateur enregistre le nombre le nombre de clients partie en province , leur donne un temps aleatoire de sejour et leur fait revenir a la gare principal apres le temps ecouler 

2)-Billeterie : La billeterie est necessaire pour respecter les horaire voulue par les clients
Regroupe les clients : Met dans des carnets sortie et entre 
Trie les demandes urgentes et standarts 
Recupere les demandes que le planificateur n'as pas peut inserer
Injecte les clients dans le planificateur au bon moment petit a petit en respectant le desir des clients

3)-Planificateur :
Le planificateur recoit les demande de la billeterie par exemple toute les 30min 
Le planificateur organise les convoie et alterne des departs en fonction du nombre de clients et de la destination qu'il recoit, c'est pourquoi on a besoin de la billetrie pour respecter l'horaire voulu par le client.
Planifie de maniere conjointe les sortie et l'entrer en respectant
    Les plages interdites 
    Le temps de rafraichissement du portail de la gare principal 
    Les parametre imposer
Planifie les horaires de depart prevue pour les convoies en fonction de la demande recu de la billeterie 
Ameliore le plus possible les depart du convoie de la gare principale pour atteindre l'objectif modeliser par un score 
L'agenda contient les horaire prevue pour chaque convoie modifier au fuir et a mesure
