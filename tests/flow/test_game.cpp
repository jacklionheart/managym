#include "managym/flow/game.h"
#include "managym/infra/log.h"
#include "tests/managym_test.h"

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <map>
#include <set>

class TestGame : public ManagymTest {
protected:
    // Track count of each card type in each zone
    struct ZoneContents {
        std::map<std::string, int> card_counts;

        void add(const Card* card) { card_counts[card->name]++; }

        void remove(const Card* card) {
            auto it = card_counts.find(card->name);
            if (it != card_counts.end() && it->second > 0) {
                it->second--;
            }
        }

        int total() const {
            int sum = 0;
            for (const auto& [_, count] : card_counts) {
                sum += count;
            }
            return sum;
        }

        std::string toString() const {
            std::string result;
            for (const auto& [name, count] : card_counts) {
                if (count > 0) {
                    result += fmt::format("{} x{}, ", name, count);
                }
            }
            return result;
        }
    };

    struct PlayerZones {
        std::array<ZoneContents, 7> zones;
        std::set<const Card*> all_cards;

        void addCard(const Card* card, ZoneType zone) {
            zones[static_cast<int>(zone)].add(card);
            all_cards.insert(card);
        }

        void removeCard(const Card* card, ZoneType zone) {
            zones[static_cast<int>(zone)].remove(card);
            all_cards.erase(card);
        }

        int totalCards() const { return all_cards.size(); }
    };

    // Helper to count cards in a zone for a given player
    int numCardsInAllZones(const Game* game, const Player* player) {
        int total = 0;
        for (int i = 0; i < 7; i++) { // 7 zones total
            total += game->zones->size(static_cast<ZoneType>(i), player);
        }
        return total;
    }

    int totalCardsInAllZones(const Game* game) {
        int total = 0;
        for (const auto& player_ptr : game->players) {
            total += numCardsInAllZones(game, player_ptr.get());
        }
        return total;
    }

    void logZoneState(const Game* game, const std::map<const Player*, PlayerZones>& zone_tracking) {
        managym::log::debug(Category::TEST, "=== Detailed Zone State ===");
        for (const auto& player_ptr : game->players) {
            const Player* player = player_ptr.get();
            const auto& player_zones = zone_tracking.at(player);

            managym::log::debug(Category::TEST, "Player {} (id={})", player->name, player->id);
            managym::log::debug(Category::TEST, "  Total unique cards tracked: {}",
                                player_zones.totalCards());

            for (int i = 0; i < 7; i++) {
                auto zone = static_cast<ZoneType>(i);
                const auto& zone_contents = player_zones.zones[i];
                managym::log::debug(Category::TEST, "  Zone {}: {} cards: {}", int(zone),
                                    zone_contents.total(), zone_contents.toString());
            }
        }
    }

    // Check if any card appears in multiple zones
    bool checkForOverlappingZones(const Game* game,
                                  const std::map<const Player*, PlayerZones>& zone_tracking) {
        for (const auto& player_ptr : game->players) {
            const Player* player = player_ptr.get();
            std::map<const Card*, std::vector<ZoneType>> card_locations;

            for (int i = 0; i < 7; i++) {
                auto zone = static_cast<ZoneType>(i);
                game->zones->forEach([&](Card* card) { card_locations[card].push_back(zone); },
                                     zone, const_cast<Player*>(player));
            }

            for (const auto& [card, locations] : card_locations) {
                if (locations.size() > 1) {
                    managym::log::error(Category::TEST,
                                        "Card {} (id={}) appears in multiple zones:", card->name,
                                        card->id);
                    for (auto zone : locations) {
                        managym::log::error(Category::TEST, "  - Zone {}", static_cast<int>(zone));
                    }
                    return true;
                }
            }
        }
        return false;
    }

    void updateZoneTracking(const Game* game, std::map<const Player*, PlayerZones>& zone_tracking) {
        for (const auto& player_ptr : game->players) {
            const Player* player = player_ptr.get();
            auto& player_zones = zone_tracking[player];

            // Clear previous state
            player_zones = PlayerZones();

            // Rebuild current state
            for (int i = 0; i < 7; i++) {
                auto zone = static_cast<ZoneType>(i);
                game->zones->forEach([&](Card* card) { player_zones.addCard(card, zone); }, zone,
                                     const_cast<Player*>(player));
            }
        }
    }
};

TEST_F(TestGame, CardCountsPreserved) {
    PlayerConfig player_a(
        "gaea", {{"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}});
    PlayerConfig player_b = player_a;
    player_b.name = "urza";

    auto game = std::make_unique<Game>(std::vector<PlayerConfig>{player_a, player_b}, true);
    ASSERT_TRUE(game != nullptr);

    // Initialize tracking
    std::map<const Player*, PlayerZones> zone_tracking;
    updateZoneTracking(game.get(), zone_tracking);

    // Log initial state
    const int initial_total = totalCardsInAllZones(game.get());
    logZoneState(game.get(), zone_tracking);
    managym::log::debug(Category::TEST, "Initial total: {} cards", initial_total);

    const int max_steps = 10000;
    int steps = 0;
    bool game_over = false;

    while (!game_over && steps < max_steps) {
        // Log pre-step state
        managym::log::debug(Category::TEST, "=== Step {} ===", steps);
        logZoneState(game.get(), zone_tracking);

        // Take step
        game_over = game->step(0);
        steps++;

        // Update tracking
        updateZoneTracking(game.get(), zone_tracking);

        // Verify no overlapping zones
        ASSERT_FALSE(checkForOverlappingZones(game.get(), zone_tracking))
            << "Found cards in multiple zones after step " << steps;

        // Verify total card count
        const int current_total = totalCardsInAllZones(game.get());
        ASSERT_EQ(current_total, initial_total) << "Total cards changed from " << initial_total
                                                << " to " << current_total << " at step " << steps;

        // For each player, verify their total cards stayed constant
        for (const auto& player_ptr : game->players) {
            const Player* player = player_ptr.get();
            int player_total = numCardsInAllZones(game.get(), player);
            ASSERT_EQ(player_total, 60) << "Player " << player->name << " card count changed to "
                                        << player_total << " at step " << steps;
        }
    }

    ASSERT_TRUE(game_over) << "Game did not complete within " << max_steps << " steps";

    // Final verification
    logZoneState(game.get(), zone_tracking);
    const int final_total = totalCardsInAllZones(game.get());
    ASSERT_EQ(final_total, initial_total) << "Total card count changed during game";
}
