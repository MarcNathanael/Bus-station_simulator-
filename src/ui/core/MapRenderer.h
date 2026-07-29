#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Destination.h"
#include "Convoi.h"

class Simulateur;

class MapRenderer {
public:
    MapRenderer(const std::vector<Destination>& destinations);
    bool loadAssets();
    void draw(sf::RenderTarget& target, const Simulateur& sim, float fraction_visuelle);
    const Convoi* pickConvoi(sf::Vector2f mousePos, const Simulateur& sim, float fraction_visuelle) const;

private:
    const std::vector<Destination>& m_destinations;
    sf::Texture m_mapTexture, m_portailTexture, m_stationTexture, m_convoiTexture;
    sf::Sprite m_mapSprite, m_portailSprite, m_convoiSprite;
    std::vector<sf::Sprite> m_stationSprites;

    void drawRoutes(sf::RenderTarget& target);
    void drawConvois(sf::RenderTarget& target, const Simulateur& sim, float fraction_visuelle);
    sf::Vector2f getConvoiVisualPosition(const Convoi& c, bool isSortie, double temps_visuel, int duree_trajet) const;
    sf::Vector2f getGarePosition() const;
    sf::Vector2f getProvincePosition(int id) const;
    void setupSprite(sf::Sprite& sprite, const sf::Texture& texture, float targetSize);
};