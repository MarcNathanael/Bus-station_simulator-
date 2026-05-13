-- SQLite
-- SQLite
-- Destinations et durées fixes
CREATE TABLE destination (
    id INTEGER PRIMARY KEY,
    nom TEXT NOT NULL UNIQUE,      -- 'DIEGO', 'TOLIARA', etc.
    duree_trajet INTEGER NOT NULL  -- en minutes
);

-- Coopératives
CREATE TABLE cooperative (
    id INTEGER PRIMARY KEY,
    nom TEXT NOT NULL
);

-- Voitures (32 places max)
CREATE TABLE voiture (
    id INTEGER PRIMARY KEY,
    id_cooperative INTEGER NOT NULL,
    nb_places_max INTEGER NOT NULL DEFAULT 32,
    nb_places_libres INTEGER NOT NULL DEFAULT 32,
    position INTEGER NOT NULL DEFAULT 7, -- Localisation actuelle
    destination_id INTEGER,
    etat TEXT NOT NULL DEFAULT 'EN_ATTENTE_GARE', -- enum EtatVoture
    FOREIGN KEY (id_cooperative) REFERENCES cooperative(id),
    FOREIGN KEY (destination_id) REFERENCES destination(id)
);

-- Plages horaires interdites (une table globale)
CREATE TABLE plage_interdite (
    id INTEGER PRIMARY KEY,
    heure_debut INTEGER NOT NULL,  -- minutes depuis minuit
    heure_fin INTEGER NOT NULL
);

-- Départs planifiés (table centrale de la planification)
CREATE TABLE depart (
    id INTEGER PRIMARY KEY,
    id_voiture INTEGER NOT NULL,
    horaire_depart INTEGER NOT NULL,   -- en minutes depuis le début de la simulation
    destination_id INTEGER NOT NULL,
    type_depart TEXT NOT NULL DEFAULT 'aller', -- 'aller' ou 'retour'
    FOREIGN KEY (id_voiture) REFERENCES voiture(id),
    FOREIGN KEY (destination_id) REFERENCES destination(id)
);

-- File de départ au portail (état ordonné)
CREATE TABLE file_depart (
    position INTEGER PRIMARY KEY AUTOINCREMENT, -- ordre dans la file
    id_voiture INTEGER NOT NULL,
    horaire_prevue INTEGER,
    FOREIGN KEY (id_voiture) REFERENCES voiture(id)
);

-- Clients
CREATE TABLE client (
    id INTEGER PRIMARY KEY,
    nom TEXT NOT NULL,
    cin TEXT NOT NULL UNIQUE,
    telephone TEXT
);

-- Billets
CREATE TABLE billet (
    id INTEGER PRIMARY KEY,
    id_client INTEGER NOT NULL,
    id_voiture INTEGER NOT NULL,
    horaire INTEGER NOT NULL,   -- horaire du départ
    etat TEXT NOT NULL DEFAULT 'RESERVE', -- EtatBillet
    destination_id INTEGER NOT NULL,
    FOREIGN KEY (destination_id) REFERENCES destination(id)
    FOREIGN KEY (id_client) REFERENCES client(id),
    FOREIGN KEY (id_voiture) REFERENCES voiture(id)
);
