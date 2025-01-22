#pragma once

// Corresponding header
// (No "game_display.cpp" include needed here, it's the .h for that .cpp)

// Headers in same directory
// (none)

// Any other managym headers
#include "managym/agent/observation.h"

// 3rd Party
#include <SFML/Graphics.hpp>

// Built-in
#include <string>

struct GameDisplay {
    GameDisplay();
    void processEvents();
    bool isOpen() const { return window.isOpen(); }

    // Render the entire observation
    void render(const Observation* obs);

private:
    // Style constants
    static constexpr float CARD_ASPECT_RATIO = 63.0f / 88.0f;
    static constexpr float RELATIVE_CARD_HEIGHT = 0.15f;
    static constexpr float RELATIVE_CARD_SPACING = 0.005f;
    static constexpr float RELATIVE_MARGIN = 0.02f;

    struct LayoutMetrics {
        float card_height = 0.0f;
        float card_width = 0.0f;
        float spacing = 0.0f;
        float margin = 0.0f;
        float window_width = 0.0f;
        float window_height = 0.0f;
        float info_section_height = 0.0f;
        float top_hand_height = 0.0f;
        float top_battlefield_height = 0.0f;
        float bottom_battlefield_height = 0.0f;
        float bottom_hand_height = 0.0f;

        float getInfoSectionY() const { return 0.0f; }
        float getTopHandY() const { return info_section_height; }
        float getTopBattlefieldY() const { return getTopHandY() + top_hand_height; }
        float getBottomBattlefieldY() const {
            return getTopBattlefieldY() + top_battlefield_height;
        }
        float getBottomHandY() const {
            return getBottomBattlefieldY() + bottom_battlefield_height;
        }
    };

    // Visual theming
    struct Theme {
        // Basic colors
        sf::Color BACKGROUND_COLOR{20, 20, 30};
        sf::Color CARD_BORDER{50, 50, 60};
        sf::Color CREATURE_FILL{245, 237, 206};
        sf::Color CREATURE_GRADIENT{232, 220, 180};
        sf::Color LAND_FILL{226, 214, 185};
        sf::Color LAND_GRADIENT{209, 193, 156};
        sf::Color SECTION_BACKGROUND{30, 30, 40, 180};
        sf::Color LIFE_TOTAL_TEXT{255, 255, 255};
        sf::Color CARD_TEXT{10, 10, 10};
        sf::Color PHASE_TEXT{200, 200, 200};
        sf::Color BATTLEFIELD_DIVIDER{40, 40, 50, 100};

        // Sizing
        float CARD_CORNER_RADIUS = 0.05f;
        float CARD_BORDER_THICKNESS = 2.0f;
        float TEXT_OUTLINE_THICKNESS = 1.0f;
    };

    Theme theme;
    sf::RenderWindow window;

    // Layout
    LayoutMetrics calculateLayout() const;

    // Helpers for drawing
    void drawSection(const sf::FloatRect& bounds,
                     const sf::Color& debug_color = sf::Color::Transparent);
    void drawLifeTotals(const sf::FloatRect& bounds, const Observation* obs);
    void drawHand(const PlayerData& player_data,
                  size_t player_index,
                  const sf::FloatRect& bounds,
                  const Observation* obs);
    void drawBattlefield(const PlayerData& player_data,
                         size_t player_index,
                         const sf::FloatRect& bounds,
                         const Observation* obs);
    void drawCard(const CardData& card_data,
                  float x,
                  float y,
                  float width,
                  float height,
                  const sf::Color& color = sf::Color::White,
                  const sf::Color& gradient_color = sf::Color::White);
    void drawPermanent(const PermanentData& perm_data,
                       const CardData* card_data,
                       float x,
                       float y,
                       float width,
                       float height,
                       const sf::Color& fill_color,
                       const sf::Color& gradient_color);
    void drawRoundedRect(const sf::FloatRect& bounds,
                         const sf::Color& fill_color,
                         const sf::Color& gradient_color,
                         float corner_radius,
                         float border_thickness = 0.0f,
                         const sf::Color& border_color = sf::Color::Transparent);
    sf::Text createText(const std::string& content,
                        unsigned int character_size,
                        const sf::Color& color = sf::Color::White);
};

extern std::string FONT_PATH; // Provide or set in your .cpp
