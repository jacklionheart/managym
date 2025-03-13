#include "managym/flow/game.h"
#include "managym/infra/log.h"

#include <iostream>
#include <random>

int main(int argc, char** argv) {
    bool debug_mode = false;
    std::set<LogCat> categories;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            debug_mode = true;
        } else if (arg.substr(0, 6) == "--log=") {
            categories = parseLogCatString(arg.substr(6));
        }
    }

    initialize_logging(categories, debug_mode ? spdlog::level::debug : spdlog::level::info);

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

    // Seed for random number generator
    std::mt19937 rng;
    rng.seed(std::random_device()());
    Game game(configs, &rng);

    // Start the game loop
    try {
        std::cout << "starting game" << std::endl;
        game.play();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
