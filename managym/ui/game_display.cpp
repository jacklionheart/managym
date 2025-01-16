#include "game_display.h"

#include <format>

std::string FONT_PATH = "/Library/Fonts/Arial Unicode.ttf";

GameDisplay::GameDisplay() : window() {
    window.create(sf::VideoMode(sf::Vector2u(1024, 768)), "Managym");
    window.setFramerateLimit(60);
}

void GameDisplay::processEvents() {
    std::optional<sf::Event> event;
    while ((event = window.pollEvent())) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                window.close();
            }
        }
    }
}

GameDisplay::LayoutMetrics GameDisplay::calculateLayout() const {
    LayoutMetrics metrics;
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

void GameDisplay::drawSection(const sf::FloatRect& bounds, const sf::Color& debug_color) {
    if (debug_color.a > 0) {
        sf::RectangleShape debug_rect(bounds.size);
        debug_rect.setPosition(bounds.position);
        debug_rect.setFillColor(debug_color);
        window.draw(debug_rect);
    }
}

void GameDisplay::drawLifeTotals(const sf::FloatRect& bounds, const Observation* obs) {
    // Create background
    sf::Vector2f bg_size = {bounds.size.x * 0.2f, bounds.size.y * 0.8f};
    sf::Vector2f bg_pos = {bounds.position.x + bounds.size.x * 0.05f,
                           bounds.position.y + bounds.size.y * 0.1f};

    sf::RectangleShape background(bg_size);
    background.setPosition(bg_pos);
    background.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(background);

    std::string game_info = std::format("Turn {}", obs->turn_number);
    if (obs->action_type != ActionType::Invalid) {
        game_info += std::format(" - {}", static_cast<int>(obs->action_type));
    }

    auto game_text = createText(game_info, bounds.size.y * 0.2f, theme.PHASE_TEXT);
    game_text.setPosition(
        {bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.1f});
    window.draw(game_text);

    for (size_t i = 0; i < obs->player_states.size(); ++i) {
        const auto& player = obs->player_states[i];
        auto text = createText(std::format("Player {} - Life: {}", i, player.life_total),
                               bounds.size.y * 0.3f, theme.LIFE_TOTAL_TEXT);
        text.setPosition({bounds.position.x + bounds.size.x * 0.06f,
                          bounds.position.y + bounds.size.y * (0.15f + i * 0.4f)});
        window.draw(text);
    }
}

void GameDisplay::drawCard(const Observation::CardState& card_state, float x, float y, float width,
                           float height, const sf::Color& color, const sf::Color& gradient_color) {
    // When tapped, we'll rotate the card 90 degrees clockwise
    // We need to adjust the position so the card rotates around its top-left corner
    sf::Transform transform;
    float draw_x = x;
    float draw_y = y;

    if (card_state.tapped) {
        // Move the rotation point to the card's top-left corner
        transform.translate(sf::Vector2f(x, y));
        // Rotate 90 degrees clockwise
        transform.rotate(sf::degrees(90));
        // Adjust position so the card stays in its original grid position after rotation
        draw_x = 0;
        draw_y = 0;
    }

    sf::FloatRect card_bounds({draw_x, draw_y}, {width, height});

    // Create card background rectangle
    sf::RectangleShape background(sf::Vector2f(width, height));
    background.setPosition(sf::Vector2f(draw_x, draw_y));
    background.setFillColor(color);
    background.setOutlineThickness(theme.CARD_BORDER_THICKNESS);
    background.setOutlineColor(theme.CARD_BORDER);

    // Draw with transform if tapped
    if (card_state.tapped) {
        window.draw(background, transform);
    } else {
        window.draw(background);
    }

    // Create and position the name text
    auto name_text = createText(card_state.name, height * 0.12f, theme.CARD_TEXT);
    name_text.setPosition({draw_x + width * 0.1f, draw_y + height * 0.1f});

    // Draw with transform if tapped
    if (card_state.tapped) {
        window.draw(name_text, transform);
    } else {
        window.draw(name_text);
    }

    // Create status text including ID for debugging
    std::string status_text = "#" + std::to_string(card_state.card_id);
    if (card_state.tapped)
        status_text += " T";
    if (card_state.summoning_sick)
        status_text += " Sick";
    if (card_state.attacking)
        status_text += " Attacking";
    if (card_state.blocking)
        status_text += " Blocking";
    if (card_state.damage > 0)
        status_text += " Damage:" + std::to_string(card_state.damage);

    auto status = createText(status_text, height * 0.10f, theme.CARD_TEXT);
    status.setPosition({draw_x + width * 0.1f, draw_y + height * 0.3f});

    // Draw with transform if tapped
    if (card_state.tapped) {
        window.draw(status, transform);
    } else {
        window.draw(status);
    }
}

void GameDisplay::drawHand(const Observation::PlayerState& player_state, size_t player_index,
                           const sf::FloatRect& bounds) {
    LayoutMetrics layout = calculateLayout();

    const auto& hand_cards = player_state.zones.find(ZoneType::HAND);
    if (hand_cards == player_state.zones.end())
        return;

    float total_width = (hand_cards->second.size() * layout.card_width) +
                        ((hand_cards->second.size() - 1) * layout.spacing);
    float start_x = bounds.position.x + (bounds.size.x - total_width) / 2;
    float y = bounds.position.y + (bounds.size.y - layout.card_height) / 2;

    for (size_t i = 0; i < hand_cards->second.size(); i++) {
        float x = start_x + i * (layout.card_width + layout.spacing);
        drawCard(hand_cards->second[i], x, y, layout.card_width, layout.card_height);
    }
}

void GameDisplay::drawBattlefield(const Observation::PlayerState& player_state, size_t player_index,
                                  const sf::FloatRect& bounds) {
    // For tapped cards, we need extra vertical space for rotation
    const float TAPPED_CARD_EXTRA_SPACE = 1.5f; // Multiplier for extra space needed
    LayoutMetrics layout = calculateLayout();

    drawRoundedRect(bounds, theme.SECTION_BACKGROUND,
                    sf::Color(theme.SECTION_BACKGROUND.r, theme.SECTION_BACKGROUND.g,
                              theme.SECTION_BACKGROUND.b, 0),
                    bounds.size.y * 0.02f);

    float land_row_height = bounds.size.y * 0.4f * TAPPED_CARD_EXTRA_SPACE;
    float creature_row_height = bounds.size.y * 0.4f * TAPPED_CARD_EXTRA_SPACE;
    float card_spacing = layout.card_width * 1.1f;
    float start_x = bounds.position.x + layout.margin;

    // Draw separator line between lands and creatures
    sf::Vector2f separator_size = {bounds.size.x * 0.9f, 2.f};
    sf::Vector2f separator_pos = {bounds.position.x + bounds.size.x * 0.05f,
                                  bounds.position.y + land_row_height};

    sf::RectangleShape separator(separator_size);
    separator.setPosition(separator_pos);
    separator.setFillColor(theme.BATTLEFIELD_DIVIDER);
    window.draw(separator);

    const auto& battlefield_cards = player_state.zones.find(ZoneType::BATTLEFIELD);
    if (battlefield_cards == player_state.zones.end())
        return;

    // Split permanents into lands and creatures (simplified logic)
    std::vector<const Observation::CardState*> lands;
    std::vector<const Observation::CardState*> creatures;

    for (const auto& card : battlefield_cards->second) {
        // This is a simplified heuristic - in a real implementation you'd want to check actual card
        // types For now we'll use the same heuristic as before where IDs < 10 are lands
        if (card.card_id < 10) {
            lands.push_back(&card);
        } else {
            creatures.push_back(&card);
        }
    }

    // Draw lands row
    for (size_t i = 0; i < lands.size(); i++) {
        float x = start_x + i * card_spacing;
        float y = bounds.position.y + layout.margin;
        drawCard(*lands[i], x, y, layout.card_width, layout.card_height, theme.LAND_FILL,
                 theme.LAND_GRADIENT);
    }

    // Draw creatures row
    for (size_t i = 0; i < creatures.size(); i++) {
        float x = start_x + i * card_spacing;
        float y = bounds.position.y + land_row_height + layout.margin;
        drawCard(*creatures[i], x, y, layout.card_width, layout.card_height, theme.CREATURE_FILL,
                 theme.CREATURE_GRADIENT);
    }
}

void GameDisplay::drawRoundedRect(const sf::FloatRect& bounds, const sf::Color& fill_color,
                                  const sf::Color& gradient_color, float corner_radius,
                                  float border_thickness, const sf::Color& border_color) {
    // Main rectangle
    sf::RectangleShape rect(bounds.size);
    rect.setPosition(bounds.position);
    rect.setFillColor(fill_color);

    if (border_thickness > 0) {
        rect.setOutlineThickness(border_thickness);
        rect.setOutlineColor(border_color);
    }
    window.draw(rect);

    // Gradient overlay
    sf::RectangleShape gradient(bounds.size);
    gradient.setPosition(bounds.position);
    gradient.setFillColor(gradient_color);
    window.draw(gradient);
}

sf::Text GameDisplay::createText(const std::string& content, unsigned int character_size,
                                 const sf::Color& color) {
    static sf::Font font;
    static bool font_loaded = false;

    if (!font_loaded) {
        if (!font.openFromFile(FONT_PATH)) {
            throw std::runtime_error("Failed to load font from: " + FONT_PATH);
        }
        font_loaded = true;
    }

    sf::Text text(font, content, character_size);
    text.setFillColor(color);
    text.setOutlineColor(sf::Color(0, 0, 0, 100));
    text.setOutlineThickness(theme.TEXT_OUTLINE_THICKNESS);

    return text;
}

void GameDisplay::render(const Observation* obs) {
    window.clear(theme.BACKGROUND_COLOR);
    LayoutMetrics layout = calculateLayout();

    // Create all bounds using Vector2f for position and size
    sf::FloatRect info_section_bounds({0, layout.getInfoSectionY()},
                                      {layout.window_width, layout.info_section_height});

    sf::FloatRect top_hand_bounds({0, layout.getTopHandY()},
                                  {layout.window_width, layout.top_hand_height});

    sf::FloatRect top_battlefield_bounds({0, layout.getTopBattlefieldY()},
                                         {layout.window_width, layout.top_battlefield_height});

    sf::FloatRect bottom_battlefield_bounds(
        {0, layout.getBottomBattlefieldY()},
        {layout.window_width, layout.bottom_battlefield_height});

    sf::FloatRect bottom_hand_bounds({0, layout.getBottomHandY()},
                                     {layout.window_width, layout.bottom_hand_height});

    drawLifeTotals(info_section_bounds, obs);
    drawHand(obs->player_states[1], 1, top_hand_bounds);
    drawBattlefield(obs->player_states[1], 1, top_battlefield_bounds);
    drawBattlefield(obs->player_states[0], 0, bottom_battlefield_bounds);
    drawHand(obs->player_states[0], 0, bottom_hand_bounds);

    window.display();
}