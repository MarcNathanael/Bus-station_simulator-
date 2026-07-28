-- =================================================================================
-- SCRIPT D'INITIALISATION SQLite - ALIGNÉ AVEC LES DAL C++
-- =================================================================================

PRAGMA foreign_keys = ON;

-- =================================================================================
-- 1. TABLES DE CONFIGURATION & RÉFÉRENCE (Statiques)
-- =================================================================================

-- Lié à DalConfiguration
CREATE TABLE IF NOT EXISTS dal_parametres (
    cle TEXT PRIMARY KEY,
    valeur INTEGER NOT NULL
);

-- Lié à DalCooperative
CREATE TABLE IF NOT EXISTS dal_cooperatives (
    id INTEGER PRIMARY KEY,
    nom TEXT NOT NULL UNIQUE
);

-- Lié à DalDestination
-- ✅ CODE CORRIGÉ
CREATE TABLE IF NOT EXISTS dal_destinations (
     id INTEGER PRIMARY KEY,
     nom TEXT NOT NULL UNIQUE,
     duree_trajet INTEGER NOT NULL CHECK (duree_trajet > 0 OR id = 0),
     positionX REAL NOT NULL,
     positionY REAL NOT NULL
);

-- Lié à DalPlageInterdite
CREATE TABLE IF NOT EXISTS dal_plages_interdites (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    heure_debut INTEGER NOT NULL CHECK (heure_debut >= 0),
    heure_fin INTEGER NOT NULL CHECK (heure_fin >= 0)
);

-- =================================================================================
-- 2. TABLES OPÉRATIONNELLES (Mises à jour en RAM puis synchronisées)
-- =================================================================================

-- Lié à DalVoiture
-- Gère la flotte. Les IDs de destination et position peuvent être nuls selon l'état.
CREATE TABLE IF NOT EXISTS dal_voitures (
    id INTEGER PRIMARY KEY,
    id_coop INTEGER NOT NULL,
    id_destination INTEGER,
    id_position INTEGER,
    capacite_max INTEGER NOT NULL CHECK (capacite_max > 0),
    places_libres INTEGER NOT NULL CHECK (places_libres >= 0),
    etat INTEGER NOT NULL DEFAULT 0,
    horaire_depart INTEGER,
    heure_arrivee REAL NOT NULL DEFAULT -1.0,

    FOREIGN KEY (id_coop) REFERENCES dal_cooperatives(id) ON DELETE RESTRICT,
    FOREIGN KEY (id_destination) REFERENCES dal_destinations(id) ON DELETE SET NULL
);

-- Lié à DalClient
-- Représente la file d'attente des clients générés
CREATE TABLE IF NOT EXISTS dal_clients_attente (
    id INTEGER PRIMARY KEY,
    destination_id INTEGER NOT NULL,
    t_min INTEGER NOT NULL,
    t_max INTEGER NOT NULL,
    priorite INTEGER NOT NULL DEFAULT 0 CHECK (priorite IN (0, 1)), -- 0: Normal, 1: Urgent

    FOREIGN KEY (destination_id) REFERENCES dal_destinations(id) ON DELETE CASCADE
);

-- =================================================================================
-- 3. TABLES D'HISTORISATION (Insert-Only, archives de la journée)
-- =================================================================================

-- Lié à DalConvoi
-- Archive les convois validés ou terminés.
CREATE TABLE IF NOT EXISTS dal_historique_convois (
    id_metier INTEGER PRIMARY KEY, -- Clé primaire explicite, indexée automatiquement (B-Tree)
    horaire_depart_reel INTEGER NOT NULL,
    type_direction TEXT NOT NULL CHECK (type_direction IN ('SORTIE', 'ENTREE')),
    destination_origine_id INTEGER NOT NULL,
    nb_voitures INTEGER NOT NULL CHECK (nb_voitures >= 0),
    --un boolean
    contient_urgence INTEGER NOT NULL DEFAULT 0 CHECK (contient_urgence IN (0, 1)),
    etat_final INTEGER NOT NULL,
    id_region INTEGER NOT NULL
);

-- Lié à DalBillet
-- Archive les ventes. L'ID est autoincrementé car non fourni dans l'INSERT C++.
CREATE TABLE IF NOT EXISTS dal_historique_billets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    client_id INTEGER NOT NULL,
    voiture_id INTEGER NOT NULL,
    heure_depart_min INTEGER NOT NULL,
    heure_depart_max INTEGER NOT NULL,
    prix REAL NOT NULL CHECK (prix >= 0.0),

    FOREIGN KEY (voiture_id) REFERENCES dal_voitures(id) ON DELETE RESTRICT
);

-- =================================================================================
-- 4. INDEX DE PERFORMANCE POUR LES REQUÊTES SPÉCIFIQUES DES DAL
-- =================================================================================

-- Pour accélérer DalConvoi::compter_convois_journee (WHERE horaire_depart_reel >= ? ...)
-- on lie horaire_depart_reel avec un index de sql afin que lorsque DalConvoi appelle sa fonction , sql n'auras plus a faire un full scan 
-- elle n'est utile qu'a sql et non a cpp
CREATE INDEX IF NOT EXISTS idx_convois_horaire ON dal_historique_convois(horaire_depart_reel);

-- Pour accélérer DalBillet::compter_billets_vendus_journee (WHERE heure_depart >= ? ...)
CREATE INDEX IF NOT EXISTS idx_billets_horaire ON dal_historique_billets(heure_depart_min);