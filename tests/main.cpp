#include "managym/infra/log.h"
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    bool debug_mode = false;
    std::set<managym::log::Category> categories;
    
    // Process our flags before GoogleTest
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--debug") {
            debug_mode = true;
            // Remove flag
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--;
            i--;
        }
        else if (arg.substr(0, 6) == "--log=") {
            categories = managym::log::parseCategoryString(arg.substr(6));
            // Remove flag
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--;
            i--;
        }
    }

    managym::log::initialize(categories, debug_mode ? spdlog::level::debug : spdlog::level::info);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}