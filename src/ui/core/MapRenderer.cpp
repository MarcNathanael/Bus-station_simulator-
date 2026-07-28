#include "MapRenderer.h"
#include "Simulateur.h"
#include "Convoi.h"
#include <cmath>
#include <iostream>

MapRenderer::MapRenderer(const std::vector<Destination>& destinations)
    : m_destinations(destinations) {}

bool MapRenderer::loadAssets() {
    if (!m_mapTexture.loadFromFile("assets/maps/map.png")) return false;
    if (!m_portailTexture.loadFromFile("assets/icons/portail.png")) return false;
    if (!m_stationTexture.loadFromFile("assets/icons/station.png")) return false;
    if (!m_convoiTexture.loadFromFile("assets/icons/convoi_sortie.png")) return false;

    m_mapSprite.setTexture(m_mapTexture);
    
    sf::Vector2f portailOrigin(m_portailTexture.getSize().x / 2.0f, m_portailTexture.getSize().y / 2.0f);
    m_portailSprite.setTexture(m_portailTexture);
    m_portailSprite.setOrigin(portailOrigin);
    m_portailSprite.setPosition(getGarePosition());

    sf::Vector2f convoiOrigin(m_convoiTexture.getSize().x / 2.0f, m_convoiTexture.getSize().y / 2.0f);
    m_convoiSprite.setTexture(m_convoiTexture);
    m_convoiSprite.setOrigin(convoiOrigin);

    sf::Vector2f stationOrigin(m_stationTexture.getSize().x / 2.0f, m_stationTexture.getSize().y / 2.0f);
    for (const auto& dest : m_destinations) {
        if (dest.get_id() == 0) continue;
        sf::Sprite sprite;
        sprite.setTexture(m_stationTexture);
        sprite.setOrigin(stationOrigin);
        sprite.setPosition(getProvincePosition(dest.get_id()));
        m_stationSprites.push_back(sprite);
    }
    return true;
}

sf::Vector2f MapRenderer::getGarePosition() const {
    for (const auto& dest : m_destinations) {
        if (dest.get_id() == 0) return sf::Vector2f(dest.get_positionX(), dest.get_positionY());
    }
    return sf::Vector2f(640, 360);
}

sf::Vector2f MapRenderer::getProvincePosition(int id) const {
    for (const auto& dest : m_destinations) {
        if (dest.get_id() == id) return sf::Vector2f(dest.get_positionX(), dest.get_positionY());
    }
    return sf::Vector2f(0, 0);
}

void MapRenderer::drawRoutes(sf::RenderTarget& target) {
    sf::Vector2f garePos = getGarePosition();
    for (const auto& dest : m_destinations) {
        if (dest.get_id() == 0) continue;
        sf::Vector2f provPos = getProvincePosition(dest.get_id());
        sf::Vector2f dir = provPos - garePos;
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (length > 0) {
            sf::Vector2f normal(-dir.y, dir.x);
            normal /= length;
            sf::Vector2f offset = normal * 5.0f;
            
            sf::Vertex line_aller[] = {
                sf::Vertex(garePos + offset, sf::Color(20, 80, 20, 150)),
                sf::Vertex(provPos + offset, sf::Color(20, 80, 20, 150))
            };
            sf::Vertex line_retour[] = {
                sf::Vertex(garePos - offset, sf::Color(80, 50, 20, 150)),
                sf::Vertex(provPos - offset, sf::Color(80, 50, 20, 150))
            };
            target.draw(line_aller, 2, sf::Lines);
            target.draw(line_retour, 2, sf::Lines);
        }
    }
}

// SEULE SOURCE DE VÉRITÉ POUR LA POSITION D'UN CONVOI
sf::Vector2f MapRenderer::getConvoiVisualPosition(const Convoi& c, bool isSortie, double temps_visuel, int duree_trajet) const {
    sf::Vector2f garePos = getGarePosition();
    sf::Vector2f provPos = getProvincePosition(c.get_id_region());
    sf::Vector2f dir = provPos - garePos;
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (length <= 0) return garePos;

    sf::Vector2f normal(-dir.y, dir.x);
    normal /= length;
    sf::Vector2f offset = normal * 5.0f;

    float progress = 0.0f;
    if (isSortie) {
        progress = static_cast<float>((temps_visuel - c.get_horaire_prevue()) / duree_trajet);
    } else {
        progress = static_cast<float>((temps_visuel - (c.get_horaire_prevue() - duree_trajet)) / duree_trajet);
    }

    // Bornage pour éviter que le bus ne dépasse la station visuellement
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    if (isSortie) {
        return garePos + offset + (dir * progress);
    } else {
        return provPos - offset + (dir * (progress - 1.0f));
    }
}

void MapRenderer::drawConvois(sf::RenderTarget& target, const Simulateur& sim, float fraction_visuelle) {
    double temps_visuel = sim.get_temps_continu() + fraction_visuelle;
    const auto& durees = sim.get_durees_trajet();

    // 1. Convois de SORTIE
    for (const Convoi& c : sim.get_convois_sortie()) {
        if (c.get_etat() == EtatConvoi::EN_TRANSIT) {
            int id_prov = c.get_id_region();
            if (durees.count(id_prov) > 0) {
                int duree_trajet = durees.at(id_prov);
                sf::Vector2f pos = getConvoiVisualPosition(c, true, temps_visuel, duree_trajet);
                
                m_convoiSprite.setPosition(pos);
                if (c.contient_urgence()) m_convoiSprite.setColor(sf::Color(200, 20, 20));
                else m_convoiSprite.setColor(sf::Color(20, 200, 20));
                
                target.draw(m_convoiSprite);
            }
        }
    }

    // 2. Convois d'ENTRÉE
    for (const Convoi& c : sim.get_convois_entree()) {
        if (c.get_etat() == EtatConvoi::EN_TRANSIT) {
            int id_prov = c.get_id_region();
            if (durees.count(id_prov) > 0) {
                int duree_trajet = durees.at(id_prov);
                sf::Vector2f pos = getConvoiVisualPosition(c, false, temps_visuel, duree_trajet);
                
                m_convoiSprite.setPosition(pos);
                if (c.contient_urgence()) m_convoiSprite.setColor(sf::Color(200, 20, 20));
                else m_convoiSprite.setColor(sf::Color(200, 130, 20));
                
                target.draw(m_convoiSprite);
            }
        }
    }
}

const Convoi* MapRenderer::pickConvoi(sf::Vector2f mousePos, const Simulateur& sim, float fraction_visuelle) const {
    double temps_visuel = sim.get_temps_continu() + fraction_visuelle;
    const auto& durees = sim.get_durees_trajet();

    for (const Convoi& c : sim.get_convois_sortie()) {
        if (c.get_etat() == EtatConvoi::EN_TRANSIT && durees.count(c.get_id_region()) > 0) {
            int duree_trajet = durees.at(c.get_id_region());
            sf::Vector2f pos = getConvoiVisualPosition(c, true, temps_visuel, duree_trajet);
            if (std::hypot(mousePos.x - pos.x, mousePos.y - pos.y) < 15.0f) return &c;
        }
    }
    for (const Convoi& c : sim.get_convois_entree()) {
        if (c.get_etat() == EtatConvoi::EN_TRANSIT && durees.count(c.get_id_region()) > 0) {
            int duree_trajet = durees.at(c.get_id_region());
            sf::Vector2f pos = getConvoiVisualPosition(c, false, temps_visuel, duree_trajet);
            if (std::hypot(mousePos.x - pos.x, mousePos.y - pos.y) < 15.0f) return &c;
        }
    }
    return nullptr;
}

void MapRenderer::draw(sf::RenderTarget& target, const Simulateur& sim, float fraction_visuelle) {
    target.draw(m_mapSprite);
    drawRoutes(target);
    drawConvois(target, sim, fraction_visuelle);
    for (const auto& sprite : m_stationSprites) target.draw(sprite);
    target.draw(m_portailSprite);
}