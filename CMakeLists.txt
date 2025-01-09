cmake_minimum_required(VERSION 3.14)
project(managym VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Better clangd support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# Find required packages
find_package(spdlog REQUIRED)
find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)

# Include directories - make them SYSTEM to silence warnings from external code
include_directories(SYSTEM
    ${CMAKE_SOURCE_DIR}
    /opt/homebrew/include
)

# Add Homebrew lib directories
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/lib")

# Collect all source files recursively
file(GLOB_RECURSE SOURCES "managym/**/*.cpp")

# Create library target with proper include directories
add_library(managym_lib ${SOURCES})
target_include_directories(managym_lib PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(managym_lib PUBLIC 
    spdlog::spdlog
    sfml-system
    sfml-window
    sfml-graphics
)

# Create executable target
add_executable(managym managym/main.cpp)
target_link_libraries(managym PRIVATE 
    managym_lib 
    spdlog::spdlog
    sfml-system
    sfml-window
    sfml-graphics
)
target_include_directories(managym PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Custom target for running the main program
add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/managym
    DEPENDS managym
)