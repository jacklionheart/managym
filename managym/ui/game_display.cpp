#include "game_display.h"

#include <SFML/Graphics.hpp>

#include <algorithm> // for std::sort
#include <format>
#include <optional>

std::string FONT_PATH = "/Library/Fonts/Arial Unicode.ttf";

// Helper to sort PlayerData by player_index
static std::vector<PlayerData> getPlayersByIndex(const Observation* obs) {
    std::vector<PlayerData> out;
    out.reserve(obs->players.size());
    for (const auto& [player_id, pdat] : obs->players) {
        out.push_back(pdat);
    }
    std::sort(out.begin(), out.end(),
              [](const PlayerData& a, const PlayerData& b) { return a.id < b.id; });
    return out;
}

// Constructor
GameDisplay::GameDisplay() : window() {
    window.create(sf::VideoMode(sf::Vector2u(1024, 768)), "Managym");
    window.setFramerateLimit(60);
}

// Poll events
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

// Calculate layout
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

// Debug rectangle
void GameDisplay::drawSection(const sf::FloatRect& bounds, const sf::Color& debug_color) {
    if (debug_color.a > 0) {
        sf::RectangleShape debug_rect(bounds.size);
        debug_rect.setPosition(bounds.position);
        debug_rect.setFillColor(debug_color);
        window.draw(debug_rect);
    }
}

// Draw top-level info: Turn number, phase, step, player life totals, etc.
void GameDisplay::drawLifeTotals(const sf::FloatRect& bounds, const Observation* obs) {
    if (!obs) {
        return;
    }

    // Create background block
    sf::Vector2f bg_size = {bounds.size.x * 0.2f, bounds.size.y * 0.8f};
    sf::Vector2f bg_pos = {bounds.position.x + bounds.size.x * 0.05f,
                           bounds.position.y + bounds.size.y * 0.1f};

    sf::RectangleShape background(bg_size);
    background.setPosition(bg_pos);
    background.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(background);

    // Build game info
    std::string game_info = std::format("Turn {}", obs->turn.turn_number);
    game_info += std::format(" | Phase {}, Step {}", static_cast<int>(obs->turn.phase),
                             static_cast<int>(obs->turn.step));
    game_info +=
        std::format(" | ActionSpace: {}", static_cast<int>(obs->action_space.action_space_type));

    auto game_text =
        createText(game_info, static_cast<unsigned int>(bounds.size.y * 0.2f), theme.PHASE_TEXT);
    game_text.setPosition(
        {bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.1f});
    window.draw(game_text);

    // Sort players by index
    auto sorted_players = getPlayersByIndex(obs);
    for (size_t i = 0; i < sorted_players.size(); ++i) {
        const auto& pdat = sorted_players[i];
        std::string pinfo = std::format("Player (ID={}): Life={}", pdat.id, pdat.life);
        auto text = createText(pinfo, static_cast<unsigned int>(bounds.size.y * 0.3f),
                               theme.LIFE_TOTAL_TEXT);
        text.setPosition({bounds.position.x + bounds.size.x * 0.06f,
                          bounds.position.y + bounds.size.y * (0.15f + i * 0.4f)});
        window.draw(text);
    }
}

// We only have registry_key, no real card name in `CardData`.
// We'll display "Card #id (reg=R)" or similar.
void GameDisplay::drawCard(const CardData& card_data, float x, float y, float width, float height,
                           const sf::Color& color, const sf::Color& gradient_color) {
    // No tapped logic here because we do not know if a card in hand is tapped.
    // Typically only permanents can be tapped. We'll see tapped in drawPermanent() instead.

    sf::RectangleShape rect(sf::Vector2f(width, height));
    rect.setPosition(sf::Vector2f(x, y));
    rect.setFillColor(color);
    rect.setOutlineThickness(theme.CARD_BORDER_THICKNESS);
    rect.setOutlineColor(theme.CARD_BORDER);
    window.draw(rect);

    // We can mention "id" and "registry_key" and "mana_cost"
    std::string name_text = std::format("Card #{} (reg={})", card_data.id, card_data.registry_key);
    auto text_obj =
        createText(name_text, static_cast<unsigned int>(height * 0.12f), theme.CARD_TEXT);
    text_obj.setPosition({x + width * 0.08f, y + height * 0.1f});
    window.draw(text_obj);

    // If you want to show the mana cost array, do something:
    // "ManaCost: W={}, U={}, B={}, R={}, G={}, C={}"
    // Or skip if you prefer. We'll skip for brevity.
}

// Draw a permanent, which uses `PermanentData` to check tapped, damage, etc.
void GameDisplay::drawPermanent(const PermanentData& perm_data,
                                const CardData* card_data, // can be null if not found
                                float x, float y, float width, float height,
                                const sf::Color& fill_color, const sf::Color& gradient_color) {
    // We decide whether to rotate if tapped.
    bool tapped = perm_data.tapped;
    sf::Transform transform;
    float draw_x = x;
    float draw_y = y;

    if (tapped) {
        // rotate around top-left corner
        transform.translate(sf::Vector2f(x, y));
        transform.rotate(sf::degrees(90));
        draw_x = 0;
        draw_y = 0;
    }

    sf::RectangleShape background(sf::Vector2f(width, height));
    background.setPosition(sf::Vector2f(draw_x, draw_y));
    background.setFillColor(fill_color);
    background.setOutlineThickness(theme.CARD_BORDER_THICKNESS);
    background.setOutlineColor(theme.CARD_BORDER);

    if (tapped) {
        window.draw(background, transform);
    } else {
        window.draw(background);
    }

    // We'll label it: "Perm #id"
    // If we have a registry_key, show it. Otherwise skip.
    std::string perm_title = std::format("Perm #{}", perm_data.id);
    if (card_data) {
        perm_title += std::format(" (reg={})", card_data->registry_key);
    }
    auto name_text =
        createText(perm_title, static_cast<unsigned int>(height * 0.12f), theme.CARD_TEXT);
    name_text.setPosition({draw_x + width * 0.1f, draw_y + height * 0.1f});
    if (tapped) {
        window.draw(name_text, transform);
    } else {
        window.draw(name_text);
    }

    // Show some permanent stats: power/toughness, damage, summoning sick
    // Summoning sick is a bool, let's label it "Sick" if true
    // Also we'll show if tapped or not
    std::string stat_text = "";
    if (card_data->power != 0 || card_data->toughness != 0) {
        stat_text += std::format("({}/{}) ", card_data->power, card_data->toughness);
    }
    if (perm_data.damage > 0) {
        stat_text += std::format("DMG:{} ", perm_data.damage);
    }
    if (perm_data.is_summoning_sick) {
        stat_text += "Sick ";
    }
    if (tapped) {
        stat_text += "TAPPED ";
    }

    auto stats_obj =
        createText(stat_text, static_cast<unsigned int>(height * 0.10f), theme.CARD_TEXT);
    stats_obj.setPosition({draw_x + width * 0.1f, draw_y + height * 0.3f});
    if (tapped) {
        window.draw(stats_obj, transform);
    } else {
        window.draw(stats_obj);
    }
}

void GameDisplay::drawHand(const PlayerData& player_data, size_t player_index,
                           const sf::FloatRect& bounds, const Observation* obs) {
    if (!obs)
        return;

    LayoutMetrics layout = calculateLayout();

    // Collect all "CardData" in zone=ZoneType::HAND for this player
    // We'll sort them by ID just for stable ordering.
    std::vector<int> hand_object_ids;
    for (const auto& [obj_id, cdat] : obs->cards) {
        if (cdat.zone == ZoneType::HAND && cdat.owner_id == player_data.id) {
            hand_object_ids.push_back(obj_id);
        }
    }
    std::sort(hand_object_ids.begin(), hand_object_ids.end());

    float total_width =
        (hand_object_ids.size() * layout.card_width) +
        ((hand_object_ids.size() > 0 ? hand_object_ids.size() - 1 : 0) * layout.spacing);
    float start_x = bounds.position.x + (bounds.size.x - total_width) / 2.f;
    float y = bounds.position.y + (bounds.size.y - layout.card_height) / 2.f;

    for (size_t i = 0; i < hand_object_ids.size(); i++) {
        int oid = hand_object_ids[i];
        auto cd_it = obs->cards.find(oid);
        if (cd_it == obs->cards.end()) {
            // If no CardData is found, skip
            // (This can happen if there's a weird partial observation.)
            continue;
        }
        float x = start_x + i * (layout.card_width + layout.spacing);
        const CardData& cdat = cd_it->second;

        drawCard(cdat, x, y, layout.card_width, layout.card_height,
                 theme.LAND_FILL, // some color
                 theme.LAND_GRADIENT);
    }
}

void GameDisplay::drawBattlefield(const PlayerData& player_data, size_t player_index,
                                  const sf::FloatRect& bounds, const Observation* obs) {
    if (!obs)
        return;

    const float TAPPED_CARD_EXTRA_SPACE = 1.5f;
    LayoutMetrics layout = calculateLayout();

    drawRoundedRect(bounds, theme.SECTION_BACKGROUND,
                    sf::Color(theme.SECTION_BACKGROUND.r, theme.SECTION_BACKGROUND.g,
                              theme.SECTION_BACKGROUND.b, 0),
                    bounds.size.y * 0.02f);

    float land_row_height = bounds.size.y * 0.4f * TAPPED_CARD_EXTRA_SPACE;
    float creature_row_height = bounds.size.y * 0.4f * TAPPED_CARD_EXTRA_SPACE;
    float card_spacing = layout.card_width * 1.1f;
    float start_x = bounds.position.x + layout.margin;

    // Separator line
    sf::Vector2f separator_size = {bounds.size.x * 0.9f, 2.f};
    sf::Vector2f separator_pos = {bounds.position.x + bounds.size.x * 0.05f,
                                  bounds.position.y + land_row_height};
    sf::RectangleShape separator(separator_size);
    separator.setPosition(separator_pos);
    separator.setFillColor(theme.BATTLEFIELD_DIVIDER);
    window.draw(separator);

    // Gather all permanent IDs for this player (battlefield)
    std::vector<int> perm_ids;
    for (const auto& [perm_id, pdat] : obs->permanents) {
        // Check the base object
        auto base_it = obs->cards.find(perm_id);
        if (base_it == obs->cards.end()) {
            // no base object => skip
            continue;
        }
        const auto& base = base_it->second;
        if (base.zone == ZoneType::BATTLEFIELD && base.owner_id == player_data.id) {
            perm_ids.push_back(perm_id);
        }
    }
    std::sort(perm_ids.begin(), perm_ids.end());

    // We'll do a trivial example: if perm_id < 10 => "land", else "creature"
    // In a real system you'd use registry_key or subtypes to check.
    // We only have `registry_key` here. Let's do:
    //    if cdata.registry_key < 100 => land, else creature
    std::vector<int> lands;
    std::vector<int> creatures;
    for (int pid : perm_ids) {
        auto cd_it = obs->cards.find(pid);
        if (cd_it == obs->cards.end()) {
            // No CardData => skip
            // Possibly a weird partial observation
            continue;
        }
        const auto& cdat = cd_it->second;
        if (cdat.registry_key < 100) {
            lands.push_back(pid);
        } else {
            creatures.push_back(pid);
        }
    }

    // Draw lands row
    for (size_t i = 0; i < lands.size(); i++) {
        int perm_id = lands[i];
        const auto& pdat = obs->permanents.at(perm_id);

        // Also get CardData if you want to reference registry_key, etc.
        auto cd_it = obs->cards.find(perm_id);
        const CardData* cdat = (cd_it == obs->cards.end() ? nullptr : &cd_it->second);

        float x = start_x + i * card_spacing;
        float y = bounds.position.y + layout.margin;
        drawPermanent(pdat, cdat, x, y, layout.card_width, layout.card_height, theme.LAND_FILL,
                      theme.LAND_GRADIENT);
    }

    // Draw creatures row
    for (size_t i = 0; i < creatures.size(); i++) {
        int perm_id = creatures[i];
        const auto& pdat = obs->permanents.at(perm_id);

        auto cd_it = obs->cards.find(perm_id);
        const CardData* cdat = (cd_it == obs->cards.end() ? nullptr : &cd_it->second);

        float x = start_x + i * card_spacing;
        float y = bounds.position.y + land_row_height + layout.margin;
        drawPermanent(pdat, cdat, x, y, layout.card_width, layout.card_height, theme.CREATURE_FILL,
                      theme.CREATURE_GRADIENT);
    }
}

// Draw a rounded rectangle
void GameDisplay::drawRoundedRect(const sf::FloatRect& bounds, const sf::Color& fill_color,
                                  const sf::Color& gradient_color, float corner_radius,
                                  float border_thickness, const sf::Color& border_color) {
    // Simple approach: no real "rounded corners," just a big rectangle.
    sf::RectangleShape rect(bounds.size);
    rect.setPosition(bounds.position);
    rect.setFillColor(fill_color);

    if (border_thickness > 0) {
        rect.setOutlineThickness(border_thickness);
        rect.setOutlineColor(border_color);
    }
    window.draw(rect);

    // Basic "gradient" overlay
    sf::RectangleShape gradient(bounds.size);
    gradient.setPosition(bounds.position);
    gradient.setFillColor(gradient_color);
    window.draw(gradient);
}

// Create text object
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

    if (!obs) {
        window.display();
        return;
    }

    LayoutMetrics layout = calculateLayout();

    // bounding rectangles
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

    // Draw overall game info
    drawLifeTotals(info_section_bounds, obs);

    // We assume exactly two players: index=0 => top seat, index=1 => bottom seat
    auto sorted_players = getPlayersByIndex(obs);
    if (sorted_players.size() >= 2) {
        // top
        drawHand(sorted_players[0], 0, top_hand_bounds, obs);
        drawBattlefield(sorted_players[0], 0, top_battlefield_bounds, obs);

        // bottom
        drawBattlefield(sorted_players[1], 1, bottom_battlefield_bounds, obs);
        drawHand(sorted_players[1], 1, bottom_hand_bounds, obs);
    }

    window.display();
}
