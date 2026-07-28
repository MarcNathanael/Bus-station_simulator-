#include "Configuration.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <filesystem> // Pour std::filesystem

bool Configuration::charger(const std::string& dossier) {
    if (!std::filesystem::exists(dossier)) {
        throw std::runtime_error("Dossier de configuration introuvable ! Chemin essaye : " 
                                 + std::filesystem::absolute(dossier).string());
    }
    try {
        parser_parametres(dossier + "/parametres.csv");
        parser_destinations(dossier + "/destinations.csv");
        parser_cooperatives(dossier + "/cooperatives.csv");
        parser_voitures(dossier + "/voitures.csv");
        parser_plages(dossier + "/plages_interdites.csv");
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
        
        std::stringstream ss(ligne);
        std::string token;
        std::vector<std::string> champs;

        while (std::getline(ss, token, ',')) {
            champs.push_back(token);
        }

        // Sécurité : on s'assure qu'on a bien 5 champs
        if (champs.size() < 5) continue;

        int id = std::stoi(champs[0]);
        std::string nom = champs[1];
        int duree = std::stoi(champs[2]);
        
        // std::stof gère les espaces initiaux (ex: " 671.7")
        float posX = std::stof(champs[3]);
        float posY = std::stof(champs[4]);

        m_destinations.emplace(id, Destination(id, nom, duree, posX, posY));
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
    
    // 1. On récupère d'abord les paramètres de temps globaux du CSV
    int t_chargement = get_parametre("temps_chargement");
    int t_dechargement = get_parametre("temps_dechargement");

    std::string ligne;
    std::getline(fichier, ligne); // ignorer en-tête

    while (std::getline(fichier, ligne)) {
        if (ligne.empty()) continue;
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

        // 2. On passe les variables de temps récoltées aux constructeurs !
        Voiture v(id, coop_id, dest_id, pos_id, cap_max, places, etat, horaire, t_chargement, t_dechargement);
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

const std::unordered_map<std::string, int>& Configuration::get_parametres() const { 
    return m_parametres; 
}

// a utiliser en haut 
int Configuration::get_parametre(const std::string& cle) const 
{
    auto it = m_parametres.find(cle);
    // si on a trouver une cle : sinon envoyer un throw (Sortie de secours, pas de return nécessaire)
    if (it != m_parametres.end()) return it->second;// c'est le parametre
    throw std::runtime_error("Paramètre inconnu : " + cle);
    //La fonction s'interrompt immédiatement.
    //Elle "lance" un objet d'erreur qui va remonter dans le programme jusqu'à trouver un bloc try { ... } catch { ... } pour gérer le problème.
}