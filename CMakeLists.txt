cmake_minimum_required(VERSION 3.21)

project(RomanianWhistTerminal
    VERSION 1.0.2
    DESCRIPTION "Terminal UI client for the Romanian Whist engine"
    LANGUAGES CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/RomanianWhistEngine/CMakeLists.txt")
    message(FATAL_ERROR
        "The Romanian Whist engine submodule is missing.\n"
        "Run: git submodule update --init --recursive")
endif()

add_subdirectory(libs/RomanianWhistEngine)

add_executable(terminal_romanian_whist
    src/main.cpp
    src/ConsoleMoveProvider.cpp
    src/TerminalRomanianWhist.cpp
)

target_compile_features(terminal_romanian_whist PRIVATE cxx_std_20)

set_target_properties(terminal_romanian_whist PROPERTIES
    OUTPUT_NAME "terminal_romanian_whist-${PROJECT_VERSION}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h
    @ONLY)

target_include_directories(terminal_romanian_whist PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Engine include directories arrive transitively through the PUBLIC
# target_include_directories on RomanianWhist::engine.
target_link_libraries(terminal_romanian_whist PRIVATE RomanianWhist::engine)

# Unversioned copy for local tooling (VSCode launch/tasks) that shouldn't need
# updating on every version bump. Released/CI binaries use the versioned name.
add_custom_command(TARGET terminal_romanian_whist POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:terminal_romanian_whist>
        $<TARGET_FILE_DIR:terminal_romanian_whist>/terminal_romanian_whist${CMAKE_EXECUTABLE_SUFFIX}
)
