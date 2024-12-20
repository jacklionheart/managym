#pragma once

#include <SFML/Graphics.hpp>

class Permanent;
class Game;
class Card;

class GameDisplay {
public:
    GameDisplay(Game* game);
    
    void processEvents();
    bool isOpen() const { return window.isOpen(); }
    bool shouldProceed() const { 
        return space_pressed || turn_timer.getElapsedTime().asSeconds() >= 5.0f; 
    }
    void render();

private:
    // Constants for relative sizing
    static constexpr float CARD_ASPECT_RATIO = 63.0f / 88.0f;  // Standard MTG card ratio
    static constexpr float RELATIVE_CARD_HEIGHT = 0.15f;       // Card height relative to window height
    static constexpr float RELATIVE_CARD_SPACING = 0.005f;     // Spacing relative to window width
    static constexpr float RELATIVE_MARGIN = 0.02f;            // Margins relative to window size

    struct LayoutMetrics {
        float card_height;
        float card_width;
        float spacing;
        float margin;
        float window_width;
        float window_height;
        
        // Sections of the window (all heights)
        float info_section_height;      // Life totals, etc.
        float top_hand_height;          // Top player's hand
        float top_battlefield_height;   // Top player's battlefield
        float bottom_battlefield_height;// Bottom player's battlefield
        float bottom_hand_height;       // Bottom player's hand

        // Calculate Y positions for each section
        float getInfoSectionY() const { return 0.0f; }
        float getTopHandY() const { return info_section_height; }
        float getTopBattlefieldY() const { return getTopHandY() + top_hand_height; }
        float getBottomBattlefieldY() const { return getTopBattlefieldY() + top_battlefield_height; }
        float getBottomHandY() const { return getBottomBattlefieldY() + bottom_battlefield_height; }
    };

    struct Theme {
        // Color scheme
        const sf::Color BACKGROUND_COLOR{20, 20, 30};  // Dark blue-black
        const sf::Color CARD_BORDER{50, 50, 60};
        
        // Card colors
        const sf::Color CREATURE_FILL{245, 237, 206};  // Warm cream
        const sf::Color CREATURE_GRADIENT{232, 220, 180};
        const sf::Color LAND_FILL{226, 214, 185};      // Slightly darker cream
        const sf::Color LAND_GRADIENT{209, 193, 156};
        
        // Section colors
        const sf::Color SECTION_BACKGROUND{30, 30, 40, 180};
        const sf::Color LIFE_TOTAL_TEXT{255, 255, 255};
        const sf::Color CARD_TEXT{10, 10, 10};
        
        // Card metrics
        const float CARD_CORNER_RADIUS = 0.05f;    // Relative to card height
        const float CARD_BORDER_THICKNESS = 2.0f;
        const float TEXT_OUTLINE_THICKNESS = 1.0f;
        
        // Battlefield sections
        const sf::Color BATTLEFIELD_DIVIDER{40, 40, 50, 100};
    } theme;

    Game* game;
    sf::RenderWindow window;
    sf::Clock turn_timer;
    bool space_pressed = false;

    LayoutMetrics calculateLayout() const;
    void drawSection(const sf::FloatRect& bounds, const sf::Color& debugColor = sf::Color::Transparent);
    void drawLifeTotals(const sf::FloatRect& bounds);
    void drawHand(Player* player, const sf::FloatRect& bounds);
    void drawBattlefield(Player* player, const sf::FloatRect& bounds);
    void drawPermanentRow(const std::vector<const Permanent*>& permanents, 
                         const sf::FloatRect& bounds,
                         const LayoutMetrics& layout);
    void drawCard(const Card* card, float x, float y, float width, float height, 
                 const sf::Color& color = sf::Color::White, const sf::Color& gradientColor = sf::Color::White);
    void drawPermanent(const Permanent* permanent, float x, float y, float width, float height);

    void drawRoundedRect(const sf::FloatRect& bounds, const sf::Color& fillColor, 
                        const sf::Color& gradientColor, float cornerRadius, 
                        float borderThickness = 0.0f, 
                        const sf::Color& borderColor = sf::Color::Transparent);
    sf::Text createText(const std::string& content, float characterSize, 
                       const sf::Color& color = sf::Color::White);

};
