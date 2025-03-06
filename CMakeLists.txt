cmake_minimum_required(VERSION 3.21)
project(managym VERSION 1.0 LANGUAGES CXX)

# Force C++20 and export compile commands.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS and Unix rpath setup
if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_INSTALL_RPATH "@loader_path")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
elseif(UNIX)
    # For Linux and other Unix-like systems, use $ORIGIN to denote the directory of the binary.
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_BUILD_RPATH "$ORIGIN")
endif()

# -------------------------------
# AddressSanitizer (macOS Debug only)
# -------------------------------
if(APPLE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling AddressSanitizer on macOS (Debug build)")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# -------------------------------
# Python Setup
# -------------------------------
set(CMAKE_FIND_DEBUG_MODE ON)
# If PYTHON_ROOT_DIR is defined in your environment, use it.
# Otherwise, if you are in a conda environment, use CONDA_PREFIX.
if(DEFINED ENV{PYTHON_ROOT_DIR})
    set(Python_ROOT_DIR $ENV{PYTHON_ROOT_DIR})
elseif(DEFINED ENV{CONDA_PREFIX})
    set(Python_ROOT_DIR $ENV{CONDA_PREFIX})
endif()

# Similarly, set the Python executable.
if(DEFINED ENV{PYTHON_EXECUTABLE})
    set(Python_EXECUTABLE $ENV{PYTHON_EXECUTABLE})
elseif(DEFINED ENV{CONDA_PREFIX})
    set(Python_EXECUTABLE "$ENV{CONDA_PREFIX}/bin/python3")
endif()

set(Python_FIND_STRATEGY LOCATION)

# Prefer the new Python finder for pybind11.
set(PYBIND11_FINDPYTHON ON CACHE BOOL "" FORCE)
set(PYBIND11_PYTHON_VERSION 3.12)

# -------------------------------
# Find Dependencies
# -------------------------------
find_package(Python 3.12 EXACT COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
    find_package(pybind11 MODULE REQUIRED)
endif()
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)

message(STATUS "Python executable: ${Python_EXECUTABLE}")
message(STATUS "Python includes:   ${Python_INCLUDE_DIRS}")
message(STATUS "Pybind11 includes: ${pybind11_INCLUDE_DIRS}")

if(APPLE)
    # Needed for creating shared libraries with undefined symbols.
    set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
elseif(UNIX)
    # (Optional) You might need additional linker flags on Linux depending on your system.
endif()

########################################
# managym_lib (Main C++ Library)
########################################
# Glob all .cpp source files under the managym folder.
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/managym/**/*.cpp")
# Remove the pybind source so it isn’t compiled into the main library.
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/managym/agent/pybind.cpp")
message(STATUS "Found source files for managym_lib: ${SOURCES}")

add_library(managym_lib SHARED ${SOURCES})

if(APPLE)
    set_target_properties(managym_lib PROPERTIES 
        INSTALL_RPATH "@loader_path"
        BUILD_RPATH "@loader_path"
    )
elseif(UNIX)
    set_target_properties(managym_lib PROPERTIES 
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN"
    )
endif()

target_link_libraries(managym_lib PUBLIC
    spdlog::spdlog
    fmt::fmt
)

# Add the project root as an include directory (assumes headers are in the project root or subfolders).
target_include_directories(managym_lib PUBLIC ${CMAKE_SOURCE_DIR})

########################################
# Python Module (_managym)
########################################
pybind11_add_module(_managym MODULE managym/agent/pybind.cpp)

if(APPLE)
    set_target_properties(_managym PROPERTIES
        INSTALL_RPATH "@loader_path"
        BUILD_RPATH "@loader_path"
    )
elseif(UNIX)
    set_target_properties(_managym PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN"
    )
endif()

target_link_libraries(_managym PRIVATE managym_lib)
target_include_directories(_managym PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Python_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
)

########################################
# Install Targets
########################################
install(TARGETS managym_lib _managym
    LIBRARY DESTINATION managym
    RUNTIME DESTINATION managym
    ARCHIVE DESTINATION managym
)

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
# CLI Executable
########################################
add_executable(managym_cli managym/main.cpp)
target_link_libraries(managym_cli PRIVATE managym_lib)
