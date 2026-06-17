#include "Client.h"

// Constructeur de reconstruction (depuis la base de données)
Client::Client(int id, int destination_id, int t_min, int t_max, bool priorite)
    : m_id(id), 
      m_destination_id(destination_id), 
      m_t_min(t_min), 
      m_t_max(t_max), 
      m_priorite(priorite) 
{}

// Constructeur de création (nouveau client du générateur)
Client::Client(int destination_id, int t_min, int t_max, bool priorite)
    : m_id(0), // 0 ou -1 pour indiquer qu'il n'est pas encore en DB
      m_destination_id(destination_id), 
      m_t_min(t_min), 
      m_t_max(t_max), 
      m_priorite(priorite) 
{}

// --- Getters ---
int Client::get_id() const             { return m_id; }
int Client::get_destination_id() const { return m_destination_id; }
int Client::get_t_min() const          { return m_t_min; }
int Client::get_t_max() const          { return m_t_max; }
bool Client::est_urgent() const        { return m_priorite; }

// --- Setters ---
void Client::set_priorite(bool priorite) { m_priorite = priorite; }
void Client::set_id(int id)              { m_id = id; }