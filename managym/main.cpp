#include "managym/flow/game.h"
#include "managym/state/card_registry.h"

#include <iostream>

int main(int argc, char** argv) {
    // Check for debug flag
    bool debug = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            debug = true;
            spdlog::set_level(spdlog::level::debug);
            break;
        }
    }
    if (!debug) {
        spdlog::set_level(spdlog::level::info);
    }

    // Register all cards that we'll use
    registerAllCards();

    // Create deck configurations
    PlayerConfig red_player("Red Mage", {
                                            {"Mountain", 12}, // 12 Mountains
                                            {"Grey Ogre", 8}  // 8 Grey Ogres
                                        });

    PlayerConfig green_player("Green Mage", {
                                                {"Forest", 12},       // 12 Forests
                                                {"Llanowar Elves", 8} // 8 Llanowar Elves
                                            });

    // Create game with these players
    std::vector<PlayerConfig> configs = {red_player, green_player};
    Game game(configs);

    // Start the game loop
    try {
        std::cout << "starting game" << std::endl;
        game.play();
    } catch (const GameOverException& e) {
        std::cout << "Game over: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}