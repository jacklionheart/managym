cmake_minimum_required(VERSION 3.14)
project(managym VERSION 1.0 LANGUAGES CXX)

# Force at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile_commands.json (helpful for clangd, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Debug info for finding Python
set(CMAKE_FIND_DEBUG_MODE ON)
set(Python_ROOT_DIR "/Users/jack/micromamba/envs/manabot")
set(Python_EXECUTABLE "/Users/jack/micromamba/envs/manabot/bin/python3")
# Usually the correct choice is 'LOCATION' for an exact path:
set(Python_FIND_STRATEGY LOCATION)

# Find Python (only the interpreter & dev headers)
find_package(Python 3.12 EXACT COMPONENTS Interpreter Development REQUIRED)

# IMPORTANT: ensure pybind11 is installed in the same conda environment.
# e.g. conda install -c conda-forge pybind11
find_package(pybind11 REQUIRED)

find_package(spdlog REQUIRED)
find_package(SFML 3.0 COMPONENTS Window Graphics REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)

message(STATUS "Python executable: ${Python_EXECUTABLE}")
message(STATUS "Python includes:   ${Python_INCLUDE_DIRS}")
message(STATUS "Pybind11 includes: ${pybind11_INCLUDE_DIRS}")

# On macOS, you do *not* want to link libpython directly.
# Instead, we pass "-undefined dynamic_lookup" so the extension
# finds CPython symbols in the already-running Python process.
if(APPLE)
    set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS
       "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
endif()

# If you want, you can explicitly set them for the extension only:
#set_target_properties(managym_py PROPERTIES
#    MACOSX_RPATH ON
#    LINK_FLAGS "-undefined dynamic_lookup"
#)

########################################
# managym_lib
########################################
# The C++ sources that do NOT use pybind or Python
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/managym/**/*.cpp"
)

# Remove the pybind code from these sources
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/managym/agent/pybind.cpp")

add_library(managym_lib SHARED ${SOURCES})
target_link_libraries(managym_lib
    PUBLIC
        spdlog::spdlog
        fmt::fmt
        SFML::Graphics
        SFML::Window
)

# This library only uses standard C++ includes
# so no direct Python includes or libraries:
target_include_directories(managym_lib PUBLIC
    ${CMAKE_SOURCE_DIR}
)

########################################
# managym_py extension
########################################
pybind11_add_module(managym_py MODULE
    managym/agent/pybind.cpp
)

# Link to your game logic library
target_link_libraries(managym_py PRIVATE managym_lib)

# We DO need the headers from Python and pybind11 to build, but no direct link to python:
target_include_directories(managym_py PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Python_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
)

########################################
# Copy & run Python tests
########################################
file(GLOB PYTHON_TEST_FILES "${CMAKE_SOURCE_DIR}/test/*.py")
file(COPY ${PYTHON_TEST_FILES} DESTINATION ${CMAKE_BINARY_DIR}/tests)

add_custom_target(pytest
    COMMAND ${CMAKE_COMMAND} -E env
            "PYTHONPATH=${CMAKE_BINARY_DIR}"   # So Python sees our local managym_py
            ${Python_EXECUTABLE} -m pytest
            ${CMAKE_BINARY_DIR}/tests
            -v
    DEPENDS managym_py
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Python tests"
)

enable_testing()
add_test(NAME python_tests
         COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target pytest
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

########################################
# CLI executable (C++ only)
########################################
add_executable(managym managym/main.cpp)
target_link_libraries(managym PRIVATE managym_lib)

########################################
# C++ tests with GoogleTest
########################################
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_SOURCE_DIR}/test/*.cpp")
add_executable(managym_test ${TEST_SOURCES})
target_link_libraries(managym_test PRIVATE
    managym_lib
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(managym_test PRIVATE
    ${CMAKE_SOURCE_DIR}
)
add_test(NAME allTests COMMAND managym_test)

# Optional convenience target to run the main program
add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/managym
    DEPENDS managym
)
