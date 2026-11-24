cmake_minimum_required(VERSION 3.21)

project(RomanianWhistTerminal
    VERSION 2.5.5
    DESCRIPTION "Terminal UI client for the Romanian Whist engine"
    LANGUAGES CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/RomanianWhistEngine/CMakeLists.txt")
    message(FATAL_ERROR
        "The Romanian Whist engine submodule is missing.\n"
        "Run: git submodule update --init --recursive")
endif()

# Off when this project is consumed by another, exactly as the engine's
# WHIST_BUILD_TESTS is - so nothing downstream builds or fetches our test
# dependencies.
option(WHIST_TERMINAL_BUILD_TESTS "Build the terminal client's test suite" ${PROJECT_IS_TOP_LEVEL})

# On only when top-level, so a project that adds this one under its own
# -Werror is not broken by our extra warnings. Warnings as errors stays off by
# default because compilers add new warnings; CI turns it on.
option(WHIST_TERMINAL_ENABLE_WARNINGS "Build the client and its tests with the project's warning set"
       ${PROJECT_IS_TOP_LEVEL})
option(WHIST_TERMINAL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# The same set as the engine. -Wall includes -Wswitch, which names an enum value
# a switch forgets. The semicolons in the generator expressions keep each flag
# a separate argument.
function(whist_terminal_enable_warnings target)
    if(NOT WHIST_TERMINAL_ENABLE_WARNINGS)
        return()
    endif()

    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wshadow;-Wpedantic;-Wnon-virtual-dtor;-Woverloaded-virtual>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>)

    if(WHIST_TERMINAL_WARNINGS_AS_ERRORS)
        target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
            $<$<CXX_COMPILER_ID:MSVC>:/WX>)
    endif()
endfunction()

# Declared before the engine is added, because the first declaration of a name
# wins and the engine's own tests declare Catch2 by tag. Their switch is read
# before the engine defines it, as it can only be on if it was set from outside.
if(WHIST_TERMINAL_BUILD_TESTS OR WHIST_BUILD_TESTS)
    include(FetchContent)

    # Pinned to a commit, not a tag, because a tag can be moved and this code is
    # built and run in the same CI job that builds the release binaries.
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG 317ac1ed4c0bb6e6b91eafc817e05c488feffcb3 # v3.16.0
    )
endif()

add_subdirectory(libs/RomanianWhistEngine)

# Everything but main(), so the tests have something to link against. The
# executable is then just an entry point.
add_library(terminal_core STATIC
    src/AppOptions.cpp
    src/Terminal.cpp
    src/CardFormat.cpp
    src/DisplayText.cpp
    src/Input.cpp
    src/GameView.cpp
    src/HandLayout.cpp
    src/Renderer.cpp
    src/Pacer.cpp
    src/SetupWizard.cpp
    src/ConsoleMoveProvider.cpp
    src/TerminalRomanianWhist.cpp
)

target_compile_features(terminal_core PUBLIC cxx_std_20)

# The Microsoft C runtime deprecates std::getenv in favour of its own
# _dupenv_s, whichever compiler reads its headers. The standard function works
# on every platform this builds for, and the code only reads from it.
target_compile_definitions(terminal_core PRIVATE
    $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>)

add_executable(terminal_romanian_whist
    src/main.cpp
)

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

# PUBLIC on both, so main.cpp and the tests see the sources and the generated
# Version.h without repeating either.
target_include_directories(terminal_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Engine include directories arrive transitively through the PUBLIC
# target_include_directories on RomanianWhist::engine.
target_link_libraries(terminal_core PUBLIC RomanianWhist::engine)

target_link_libraries(terminal_romanian_whist PRIVATE terminal_core)

whist_terminal_enable_warnings(terminal_core)
whist_terminal_enable_warnings(terminal_romanian_whist)

# Unversioned copy, so the commands in the README don't change with every
# version bump. Released/CI binaries use the versioned name.
add_custom_command(TARGET terminal_romanian_whist POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:terminal_romanian_whist>
        $<TARGET_FILE_DIR:terminal_romanian_whist>/terminal_romanian_whist${CMAKE_EXECUTABLE_SUFFIX}
)

if(WHIST_TERMINAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
