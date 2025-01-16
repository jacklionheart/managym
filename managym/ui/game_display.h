#pragma once

#include "managym/agent/observation.h"

#include <SFML/Graphics.hpp>

#include <string>

class GameDisplay {
public:
    GameDisplay();
    void processEvents();
    bool isOpen() const { return window.isOpen(); }
    void render(const Observation* obs);

private:
    // Constants for relative sizing
    static constexpr float CARD_ASPECT_RATIO = 63.0f / 88.0f;
    static constexpr float RELATIVE_CARD_HEIGHT = 0.15f;
    static constexpr float RELATIVE_CARD_SPACING = 0.005f;
    static constexpr float RELATIVE_MARGIN = 0.02f;

    struct LayoutMetrics {
        float card_height;
        float card_width;
        float spacing;
        float margin;
        float window_width;
        float window_height;

        float info_section_height;
        float top_hand_height;
        float top_battlefield_height;
        float bottom_battlefield_height;
        float bottom_hand_height;

        float getInfoSectionY() const { return 0.0f; }
        float getTopHandY() const { return info_section_height; }
        float getTopBattlefieldY() const { return getTopHandY() + top_hand_height; }
        float getBottomBattlefieldY() const {
            return getTopBattlefieldY() + top_battlefield_height;
        }
        float getBottomHandY() const { return getBottomBattlefieldY() + bottom_battlefield_height; }
    };

    struct Theme {
        const sf::Color BACKGROUND_COLOR{20, 20, 30};
        const sf::Color CARD_BORDER{50, 50, 60};
        const sf::Color CREATURE_FILL{245, 237, 206};
        const sf::Color CREATURE_GRADIENT{232, 220, 180};
        const sf::Color LAND_FILL{226, 214, 185};
        const sf::Color LAND_GRADIENT{209, 193, 156};
        const sf::Color SECTION_BACKGROUND{30, 30, 40, 180};
        const sf::Color LIFE_TOTAL_TEXT{255, 255, 255};
        const sf::Color CARD_TEXT{10, 10, 10};
        const sf::Color PHASE_TEXT{200, 200, 200};
        const sf::Color BATTLEFIELD_DIVIDER{40, 40, 50, 100};

        const float CARD_CORNER_RADIUS = 0.05f;
        const float CARD_BORDER_THICKNESS = 2.0f;
        const float TEXT_OUTLINE_THICKNESS = 1.0f;
    } theme;

    sf::RenderWindow window;

    // Layouts
    LayoutMetrics calculateLayout() const;
    void drawSection(const sf::FloatRect& bounds,
                     const sf::Color& debug_color = sf::Color::Transparent);
    void drawLifeTotals(const sf::FloatRect& bounds, const Observation* obs);
    void drawHand(const Observation::PlayerState& player_state, size_t player_index,
                  const sf::FloatRect& bounds);
    void drawBattlefield(const Observation::PlayerState& player_state, size_t player_index,
                         const sf::FloatRect& bounds);

    // Drawing primitives
    void drawCard(const Observation::CardState& card_state, float x, float y, float width,
                  float height, const sf::Color& color = sf::Color::White,
                  const sf::Color& gradient_color = sf::Color::White);
    void drawRoundedRect(const sf::FloatRect& bounds, const sf::Color& fill_color,
                         const sf::Color& gradient_color, float corner_radius,
                         float border_thickness = 0.0f,
                         const sf::Color& border_color = sf::Color::Transparent);
    sf::Text createText(const std::string& content, unsigned int character_size,
                        const sf::Color& color = sf::Color::White);
};