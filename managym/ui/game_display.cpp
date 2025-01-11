#include "game_display.h"

#include "managym/flow/game.h"
#include "managym/state/zones.h"

#include <iostream>
#include <string>

std::string FONT_PATH = "/Library/Fonts/Arial Unicode.ttf";

GameDisplay::GameDisplay(Game* game) : game(game), window(sf::VideoMode(1024, 768), "Magic Game") {
    window.setFramerateLimit(60);
}

void GameDisplay::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        } else if (event.type == sf::Event::KeyPressed) {
            switch (event.key.code) {
            case sf::Keyboard::Escape:
                window.close();
                break;
            default:
                break;
            }
        }
    }
}

GameDisplay::LayoutMetrics GameDisplay::calculateLayout() const {
    GameDisplay::LayoutMetrics metrics;
    metrics.window_width = static_cast<float>(window.getSize().x);
    metrics.window_height = static_cast<float>(window.getSize().y);

    metrics.card_height = metrics.window_height * RELATIVE_CARD_HEIGHT;
    metrics.card_width = metrics.card_height * CARD_ASPECT_RATIO;
    metrics.spacing = metrics.window_width * RELATIVE_CARD_SPACING;
    metrics.margin = metrics.window_width * RELATIVE_MARGIN;

    metrics.info_section_height = metrics.window_height * 0.1f;
    metrics.top_hand_height = metrics.card_height * 1.2f;
    metrics.bottom_hand_height = metrics.card_height * 1.2f;

    float remaining_height =
        metrics.window_height -
        (metrics.info_section_height + metrics.top_hand_height + metrics.bottom_hand_height);
    metrics.top_battlefield_height = remaining_height * 0.5f;
    metrics.bottom_battlefield_height = remaining_height * 0.5f;

    return metrics;
}

void GameDisplay::drawSection(const sf::FloatRect& bounds, const sf::Color& debugColor) {
    if (debugColor.a > 0) {
        sf::RectangleShape debug_rect(sf::Vector2f(bounds.width, bounds.height));
        debug_rect.setPosition(bounds.left, bounds.top);
        debug_rect.setFillColor(debugColor);
        window.draw(debug_rect);
    }
}

void GameDisplay::drawLifeTotals(const sf::FloatRect& bounds) {
    sf::Font font;
    if (!font.loadFromFile(FONT_PATH)) {
        throw std::runtime_error("Failed to load font");
    }

    // Background
    sf::RectangleShape background(sf::Vector2f(bounds.width * 0.2f, bounds.height * 0.8f));
    background.setFillColor(sf::Color(0, 0, 0, 180));
    background.setPosition(bounds.left + bounds.width * 0.05f, bounds.top + bounds.height * 0.1f);
    window.draw(background);

    // Life totals
    for (size_t i = 0; i < game->players.size(); ++i) {
        Player* player = game->players[i].get();
        std::string life_text = player->name + ": " + std::to_string(player->life);

        sf::Text text(life_text, font, bounds.height * 0.3f);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition(bounds.left + bounds.width * 0.06f,
                         bounds.top + bounds.height * (0.15f + i * 0.4f));

        window.draw(text);
    }
}
void GameDisplay::drawRoundedRect(const sf::FloatRect& bounds, const sf::Color& fillColor,
                                  const sf::Color& gradientColor, float cornerRadius,
                                  float borderThickness, const sf::Color& borderColor) {
    // Create shape with rounded corners
    sf::RectangleShape rect(sf::Vector2f(bounds.width, bounds.height));
    rect.setPosition(bounds.left, bounds.top);

    // Create gradient effect
    sf::RectangleShape gradient(sf::Vector2f(bounds.width, bounds.height));
    gradient.setPosition(bounds.left, bounds.top);
    gradient.setFillColor(gradientColor);

    // Add border if specified
    if (borderThickness > 0) {
        rect.setOutlineThickness(borderThickness);
        rect.setOutlineColor(borderColor);
    }

    rect.setFillColor(fillColor);

    window.draw(rect);
    window.draw(gradient);
}

sf::Text GameDisplay::createText(const std::string& content, float characterSize,
                                 const sf::Color& color) {
    static sf::Font font;
    static bool fontLoaded = false;

    if (!fontLoaded) {
        if (!font.loadFromFile(FONT_PATH)) {
            throw std::runtime_error("Failed to load font from: " + FONT_PATH);
        }
        fontLoaded = true;
    }

    sf::Text text(content, font);
    text.setCharacterSize(static_cast<unsigned int>(characterSize));
    text.setFillColor(color);
    text.setOutlineColor(sf::Color(0, 0, 0, 100));
    text.setOutlineThickness(theme.TEXT_OUTLINE_THICKNESS);

    return text;
}

void GameDisplay::drawCard(const Card* card, float x, float y, float width, float height,
                           const sf::Color& fillColor, const sf::Color& gradientColor) {
    // Draw card background with rounded corners
    sf::FloatRect cardBounds(x, y, width, height);
    drawRoundedRect(cardBounds, fillColor, gradientColor, height * theme.CARD_CORNER_RADIUS,
                    theme.CARD_BORDER_THICKNESS, theme.CARD_BORDER);

    // Draw card name with better text styling
    sf::Text nameText = createText(card->name, height * 0.12f, theme.CARD_TEXT);
    nameText.setPosition(x + width * 0.1f, y + height * 0.1f);

    // Draw card type line
    std::string typeText = card->types.isCreature() ? "Creature" : "Land";
    sf::Text type = createText(typeText, height * 0.1f, theme.CARD_TEXT);
    type.setPosition(x + width * 0.1f, y + height * 0.3f);

    // If it's a creature, draw power/toughness
    if (card->types.isCreature()) {
        std::string stats = std::to_string(card->power.value_or(0)) + "/" +
                            std::to_string(card->toughness.value_or(0));
        sf::Text statsText = createText(stats, height * 0.12f, theme.CARD_TEXT);
        statsText.setPosition(x + width * 0.7f, y + height * 0.8f);
        window.draw(statsText);
    }

    window.draw(nameText);
    window.draw(type);
}

void GameDisplay::drawPermanent(const Permanent* permanent, float x, float y, float width,
                                float height) {
    sf::Color fillColor = permanent->card->types.isLand() ? theme.LAND_FILL : theme.CREATURE_FILL;
    sf::Color gradientColor =
        permanent->card->types.isLand() ? theme.LAND_GRADIENT : theme.CREATURE_GRADIENT;

    if (permanent->tapped) {
        // Draw tapped card with rotation and shadow effect
        sf::FloatRect shadowBounds(x + 2, y + 2, height, width);
        drawRoundedRect(shadowBounds, sf::Color(0, 0, 0, 50), sf::Color(0, 0, 0, 30),
                        width * theme.CARD_CORNER_RADIUS);

        sf::Transform transform;
        transform.translate(x + width / 2, y + height / 2);
        transform.rotate(90);
        transform.translate(-height / 2, -width / 2);

        window.draw(sf::RectangleShape(), transform);
    }

    drawCard(permanent->card, x, y, width, height, fillColor, gradientColor);
}

void GameDisplay::drawHand(Player* player, const sf::FloatRect& bounds) {
    const auto& hand_cards = game->zones->constHand()->cards.at(player);
    LayoutMetrics layout = calculateLayout();

    float total_cards_width =
        (hand_cards.size() * layout.card_width) + ((hand_cards.size() - 1) * layout.spacing);
    float start_x = bounds.left + (bounds.width - total_cards_width) / 2;
    float y = bounds.top + (bounds.height - layout.card_height) / 2;

    for (size_t i = 0; i < hand_cards.size(); i++) {
        float x = start_x + i * (layout.card_width + layout.spacing);
        drawCard(hand_cards[i], x, y, layout.card_width, layout.card_height);
    }
}

void GameDisplay::drawBattlefield(Player* player, const sf::FloatRect& bounds) {
    // Draw battlefield background
    drawRoundedRect(bounds, theme.SECTION_BACKGROUND,
                    sf::Color(theme.SECTION_BACKGROUND.r, theme.SECTION_BACKGROUND.g,
                              theme.SECTION_BACKGROUND.b, 0),
                    bounds.height * 0.02f);

    // Draw separator between lands and creatures
    float landHeight = bounds.height * 0.4f;
    sf::RectangleShape separator(sf::Vector2f(bounds.width * 0.9f, 2.f));
    separator.setPosition(bounds.left + bounds.width * 0.05f, bounds.top + landHeight);
    separator.setFillColor(theme.BATTLEFIELD_DIVIDER);
    window.draw(separator);

    LayoutMetrics layout = calculateLayout();
    const auto& permanents = game->zones->constBattlefield()->permanents.at(player);

    std::vector<const Permanent*> lands, creatures;
    for (const auto& permanent : permanents) {
        if (permanent->card->types.isLand()) {
            lands.push_back(permanent.get());
        } else {
            creatures.push_back(permanent.get());
        }
    }

    float land_section_height = bounds.height * 0.4f;
    float creature_section_height = bounds.height * 0.6f;

    sf::FloatRect land_bounds(bounds.left, bounds.top, bounds.width, land_section_height);
    drawPermanentRow(lands, land_bounds, layout);

    sf::FloatRect creature_bounds(bounds.left, bounds.top + land_section_height, bounds.width,
                                  creature_section_height);
    drawPermanentRow(creatures, creature_bounds, layout);
}

void GameDisplay::drawPermanentRow(const std::vector<const Permanent*>& permanents,
                                   const sf::FloatRect& bounds, const LayoutMetrics& layout) {
    if (permanents.empty())
        return;

    float total_width =
        (permanents.size() * layout.card_width) + ((permanents.size() - 1) * layout.spacing);
    float start_x = bounds.left + (bounds.width - total_width) / 2;
    float y = bounds.top + (bounds.height - layout.card_height) / 2;

    for (size_t i = 0; i < permanents.size(); i++) {
        float x = start_x + i * (layout.card_width + layout.spacing);
        drawPermanent(permanents[i], x, y, layout.card_width, layout.card_height);
    }
}

void GameDisplay::render() {
    window.clear(theme.BACKGROUND_COLOR);

    LayoutMetrics layout = calculateLayout();

    drawLifeTotals(sf::FloatRect(0, layout.getInfoSectionY(), layout.window_width,
                                 layout.info_section_height));

    drawHand(game->players[1].get(),
             sf::FloatRect(0, layout.getTopHandY(), layout.window_width, layout.top_hand_height));
    drawBattlefield(game->players[1].get(),
                    sf::FloatRect(0, layout.getTopBattlefieldY(), layout.window_width,
                                  layout.top_battlefield_height));
    drawBattlefield(game->players[0].get(),
                    sf::FloatRect(0, layout.getBottomBattlefieldY(), layout.window_width,
                                  layout.bottom_battlefield_height));
    drawHand(game->players[0].get(), sf::FloatRect(0, layout.getBottomHandY(), layout.window_width,
                                                   layout.bottom_hand_height));

    window.display();
}