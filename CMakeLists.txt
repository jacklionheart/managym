cmake_minimum_required(VERSION 3.14)
project(managym VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Better clangd support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# Define necessary compile definitions globally
add_compile_definitions(SPDLOG_HEADER_ONLY)

find_package(spdlog REQUIRED)
find_package(SFML 3.0 COMPONENTS Window Graphics REQUIRED)
find_package(GTest REQUIRED)

# Include directories
include_directories(SYSTEM
    ${CMAKE_SOURCE_DIR}
    /opt/homebrew/include
)

# Add Homebrew lib directories
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/lib")

# Collect all source files for the main library recursively
file(GLOB_RECURSE SOURCES "managym/**/*.cpp")

# Create library target
add_library(managym_lib SHARED ${SOURCES})
target_include_directories(managym_lib PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(managym_lib PUBLIC
    SFML::Graphics
    SFML::Window
    spdlog::spdlog
)

# Create the main executable
add_executable(managym managym/main.cpp)

target_link_libraries(managym 
    PRIVATE 
        managym_lib
        spdlog::spdlog
        SFML::Graphics
        SFML::Window
)

target_include_directories(managym 
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Custom target for running the main program
add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/managym
    DEPENDS managym
)
# Collect test files from the managym_test directory
file(GLOB_RECURSE TEST_SOURCES "managym_test/*.cpp")

# Create the test executable
add_executable(managym_test ${TEST_SOURCES})
target_link_libraries(managym_test PRIVATE
    managym_lib
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(managym_test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable CTest-based testing
enable_testing()

# Add an actual test named "allTests" that runs managym_test
add_test(NAME allTests COMMAND managym_test)