#pragma once

struct GroupeClients {
    int id_destination;
    int nb_passagers;
    int t_min; // Heure de départ souhaitée au plus tôt
    int t_max; // Heure limite de tolérance (après, le client annule)
    bool est_un_retour; // true si province->gare, false si gare->province
};