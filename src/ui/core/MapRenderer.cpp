#include "MapRenderer.h"
#include "Simulateur.h"
#include <cmath>

MapRenderer::MapRenderer(const std::vector<Destination>& destinations) : m_destinations(destinations) {}

void MapRenderer::setupSprite(sf::Sprite& sprite, const sf::Texture& texture, float targetSize) {
    sprite.setTexture(texture);
    sprite.setScale(targetSize / texture.getSize().x, targetSize / texture.getSize().y);
    sprite.setOrigin(texture.getSize().x / 2.0f, texture.getSize().y / 2.0f);
}

bool MapRenderer::loadAssets() {
    if (!m_mapTexture.loadFromFile("assets/maps/map.png")) return false;
    if (!m_portailTexture.loadFromFile("assets/icons/portail.png")) return false;
    if (!m_stationTexture.loadFromFile("assets/icons/station.png")) return false;
    if (!m_convoiTexture.loadFromFile("assets/icons/convoi_sortie.png")) return false;

    m_mapSprite.setTexture(m_mapTexture);
    setupSprite(m_portailSprite, m_portailTexture, 48.0f);
    m_portailSprite.setPosition(getGarePosition());
    setupSprite(m_convoiSprite, m_convoiTexture, 36.0f);

    m_stationSprites.clear();
    for (const auto& dest : m_destinations) {
        if (dest.get_id() == 0) continue;
        sf::Sprite s;
        setupSprite(s, m_stationTexture, 32.0f);
        s.setPosition(getProvincePosition(dest.get_id()));
        m_stationSprites.push_back(s);
    }
    return true;
}

sf::Vector2f MapRenderer::getGarePosition() const {
    for (const auto& d : m_destinations) if (d.get_id() == 0) return sf::Vector2f(d.get_positionX(), d.get_positionY());
    return sf::Vector2f(640, 360);
}

sf::Vector2f MapRenderer::getProvincePosition(int id) const {
    for (const auto& d : m_destinations) if (d.get_id() == id) return sf::Vector2f(d.get_positionX(), d.get_positionY());
    return sf::Vector2f(0, 0);
}

void MapRenderer::drawRoutes(sf::RenderTarget& target) {
    sf::Vector2f garePos = getGarePosition();
    for (const auto& dest : m_destinations) {
        if (dest.get_id() == 0) continue;
        sf::Vector2f provPos = getProvincePosition(dest.get_id());
        sf::Vector2f dir = provPos - garePos;
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0) {
            sf::Vector2f normal(-dir.y, dir.x);
            normal /= len;
            sf::Vector2f offset = normal * 7.0f;
            sf::Vertex aller[] = { sf::Vertex(garePos + offset, sf::Color(0, 255, 100, 220)), sf::Vertex(provPos + offset, sf::Color(0, 255, 100, 220)) };
            sf::Vertex retour[] = { sf::Vertex(garePos - offset, sf::Color(255, 165, 0, 220)), sf::Vertex(provPos - offset, sf::Color(255, 165, 0, 220)) };
            target.draw(aller, 2, sf::Lines);
            target.draw(retour, 2, sf::Lines);
        }
    }
}

sf::Vector2f MapRenderer::getConvoiVisualPosition(const Convoi& c, bool isSortie, double t_visuel, int duree) const {
    sf::Vector2f g = getGarePosition();
    sf::Vector2f p = getProvincePosition(c.get_id_region());
    sf::Vector2f dir = p - g;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len <= 0) return g;

    sf::Vector2f normal(-dir.y, dir.x);
    normal /= len;
    sf::Vector2f offset = normal * 7.0f;

    float prog = 0.0f;
    if (isSortie) {
        prog = (t_visuel - c.get_horaire_prevue()) / duree;
        prog = std::max(0.0f, std::min(1.0f, prog));
        return g + offset + (dir * prog); // Gare -> Province
    } else {
        // Le convoi part à (horaire_prevue - duree) et arrive à horaire_prevue
        prog = (t_visuel - (c.get_horaire_prevue() - duree)) / duree;
        prog = std::max(0.0f, std::min(1.0f, prog));
        return p - offset - (dir * prog); // Province -> Gare (Corrigé)
    }
}

void MapRenderer::drawConvois(sf::RenderTarget& target, const Simulateur& sim, float frac) {
    double t_visuel = sim.get_temps_continu() + frac;
    const auto& durees = sim.get_durees_trajet();

    for (const Convoi& c : sim.get_convois_sortie()) {
        if (c.get_etat() == EtatConvoi::EN_TRANSIT && durees.count(c.get_id_region()) > 0) {
            m_convoiSprite.setPosition(getConvoiVisualPosition(c, true, t_visuel, durees.at(c.get_id_region())));
            m_convoiSprite.setColor(c.contient_urgence() ? sf::Color(255, 50, 50) : sf::Color(50, 255, 50));
            target.draw(m_convoiSprite);
        }
    }
    for (const Convoi& c : sim.get_convois_entree()) {
        if (c.get_etat() == EtatConvoi::EN_TRANSIT && durees.count(c.get_id_region()) > 0) {
            m_convoiSprite.setPosition(getConvoiVisualPosition(c, false, t_visuel, durees.at(c.get_id_region())));
            m_convoiSprite.setColor(c.contient_urgence() ? sf::Color(255, 50, 50) : sf::Color(255, 165, 0));
            target.draw(m_convoiSprite);
        }
    }
}

const Convoi* MapRenderer::pickConvoi(sf::Vector2f mousePos, const Simulateur& sim, float frac) const {
    double t_visuel = sim.get_temps_continu() + frac;
    const auto& durees = sim.get_durees_trajet();

    for (const Convoi& c : sim.get_convois_sortie()) {
        if (c.get_etat() == EtatConvoi::EN_TRANSIT && durees.count(c.get_id_region()) > 0) {
            sf::Vector2f pos = getConvoiVisualPosition(c, true, t_visuel, durees.at(c.get_id_region()));
            if (std::hypot(mousePos.x - pos.x, mousePos.y - pos.y) < 25.0f) return &c;
        }
    }
    for (const Convoi& c : sim.get_convois_entree()) {
        if (c.get_etat() == EtatConvoi::EN_TRANSIT && durees.count(c.get_id_region()) > 0) {
            sf::Vector2f pos = getConvoiVisualPosition(c, false, t_visuel, durees.at(c.get_id_region()));
            if (std::hypot(mousePos.x - pos.x, mousePos.y - pos.y) < 25.0f) return &c;
        }
    }
    return nullptr;
}

void MapRenderer::draw(sf::RenderTarget& target, const Simulateur& sim, float frac) {
    target.draw(m_mapSprite);
    drawRoutes(target);
    drawConvois(target, sim, frac);
    for (const auto& s : m_stationSprites) target.draw(s);
    target.draw(m_portailSprite);
}