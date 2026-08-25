cmake_minimum_required(VERSION 3.14)

project(RomanianWhistTerminal
    VERSION 1.0.0
    DESCRIPTION "Terminal UI client for the Romanian Whist engine"
    LANGUAGES CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/RomanianWhistEngine/CMakeLists.txt")
    message(FATAL_ERROR
        "The Romanian Whist engine submodule is missing.\n"
        "Run: git submodule update --init --recursive")
endif()

add_subdirectory(libs/RomanianWhistEngine)

add_executable(whist_terminal
    src/main.cpp
    src/ConsoleMoveProvider.cpp
    src/TerminalRomanianWhist.cpp
)

target_compile_features(whist_terminal PRIVATE cxx_std_20)

# Engine include directories arrive transitively through the PUBLIC
# target_include_directories on RomanianWhist::engine.
target_link_libraries(whist_terminal PRIVATE RomanianWhist::engine)
