# Basic setup
cmake_minimum_required(VERSION 3.14)
project(managym VERSION 1.0 LANGUAGES CXX)

# Force at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS rpath setup 
if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_INSTALL_RPATH "@loader_path")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

# Python setup
set(CMAKE_FIND_DEBUG_MODE ON)
set(Python_ROOT_DIR "/Users/jack/micromamba/envs/manabot")
set(Python_EXECUTABLE "/Users/jack/micromamba/envs/manabot/bin/python3")
set(Python_FIND_STRATEGY LOCATION)

# Find dependencies
find_package(Python 3.12 EXACT COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)
find_package(spdlog REQUIRED)
find_package(SFML 3.0 COMPONENTS Window Graphics REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)

message(STATUS "Python executable: ${Python_EXECUTABLE}")
message(STATUS "Python includes:   ${Python_INCLUDE_DIRS}")
message(STATUS "Pybind11 includes: ${pybind11_INCLUDE_DIRS}")

if(APPLE)
    set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
endif()

########################################
# managym_lib
########################################
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/managym/**/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/managym/agent/pybind.cpp")

add_library(managym_lib SHARED ${SOURCES})

if(APPLE)
    set_target_properties(managym_lib PROPERTIES 
        INSTALL_RPATH "@loader_path"
        BUILD_RPATH "@loader_path"
    )
endif()

target_link_libraries(managym_lib PUBLIC
    spdlog::spdlog
    fmt::fmt
    SFML::Graphics
    SFML::Window
)

target_include_directories(managym_lib PUBLIC ${CMAKE_SOURCE_DIR})

########################################
# managym python module
########################################
pybind11_add_module(_managym MODULE managym/agent/pybind.cpp)

if(APPLE)
    set_target_properties(_managym PROPERTIES
        INSTALL_RPATH "@loader_path"
        BUILD_RPATH "@loader_path"
    )
endif()

target_link_libraries(_managym PRIVATE managym_lib)
target_include_directories(_managym PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Python_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
)

# Install targets
install(TARGETS managym_lib _managym
    LIBRARY DESTINATION managym
    RUNTIME DESTINATION managym
    ARCHIVE DESTINATION managym)

########################################
# Testing
########################################
enable_testing()

# C++ Tests
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_SOURCE_DIR}/tests/*.cpp")
add_executable(managym_test ${TEST_SOURCES})
target_link_libraries(managym_test PRIVATE
    managym_lib
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(managym_test PRIVATE ${CMAKE_SOURCE_DIR})
add_test(NAME cpp_tests COMMAND managym_test)

# Python Tests 
add_test(NAME python_tests 
    COMMAND ${Python_EXECUTABLE} -m pytest ${CMAKE_SOURCE_DIR}/tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Combined test target
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS managym_test _managym
)

########################################
# CLI executable
########################################
add_executable(managym_cli managym/main.cpp)
target_link_libraries(managym_cli PRIVATE managym_lib)
