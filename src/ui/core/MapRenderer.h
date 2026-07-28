#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Destination.h"

// Déclaration anticipée pour éviter les inclusions circulaires
class Simulateur; 
class Convoi;

class MapRenderer {
public:
    MapRenderer(const std::vector<Destination>& destinations);
    bool loadAssets();
    // On ajoute le simulateur et la fraction visuelle pour dessiner les bus
    void draw(sf::RenderTarget& target, const Simulateur& sim, float fraction_visuelle);
    const Convoi* pickConvoi(sf::Vector2f mousePos, const Simulateur& sim, float fraction_visuelle) const;

private:
    const std::vector<Destination>& m_destinations;
    sf::Texture m_mapTexture;
    sf::Texture m_portailTexture;
    sf::Texture m_stationTexture;
    sf::Texture m_convoiTexture; // Nouveau
    
    sf::Sprite m_mapSprite;
    sf::Sprite m_portailSprite;
    sf::Sprite m_convoiSprite;   // Nouveau (un seul sprite qu'on déplace)
    std::vector<sf::Sprite> m_stationSprites;
    
    void drawRoutes(sf::RenderTarget& target);
    void drawConvois(sf::RenderTarget& target, const Simulateur& sim, float fraction_visuelle);
    void drawConvoiIcon(sf::RenderTarget& target, const Convoi& c, float progress, bool isSortie);
    
    sf::Vector2f getGarePosition() const;
    sf::Vector2f getProvincePosition(int id) const;
    sf::Vector2f getConvoiVisualPosition(const Convoi& c, bool isSortie, double temps_visuel, int duree_trajet) const;
};