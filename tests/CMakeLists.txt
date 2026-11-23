FetchContent_MakeAvailable(Catch2)

add_executable(terminal_tests
    HandLayoutTests.cpp
    AppOptionsTests.cpp
    CardFormatTests.cpp
    DisplayTextTests.cpp
    InputTests.cpp
    RendererTests.cpp
    ConsoleMoveProviderTests.cpp
    FrameTests.cpp
    GameViewTests.cpp
)

target_link_libraries(terminal_tests PRIVATE
    terminal_core
    Catch2::Catch2WithMain
)

target_include_directories(terminal_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(terminal_tests PRIVATE cxx_std_20)

whist_terminal_enable_warnings(terminal_tests)

# Cards are compared by value here as they are in the engine's suite, and half
# of those comparisons are on an optional. Catch2 renders those "{?}" whatever
# StringMaker specializations are in scope unless this is defined - and it has
# to be seen before catch_tostring.hpp, which catch_test_macros.hpp already
# pulls in, so a #define in a header of ours cannot win against include order.
target_compile_definitions(terminal_tests PRIVATE
    CATCH_CONFIG_ENABLE_OPTIONAL_STRINGMAKER
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(terminal_tests)

# Off by default: the demo's bots compare floats from std::exp and std::pow, so
# the transcript replays only on the maths library it was recorded with, which
# is the Linux CI job's. That job turns this on.
option(WHIST_TERMINAL_GOLDEN_TEST "Compare the seeded demo with its recorded transcript" OFF)

if(WHIST_TERMINAL_GOLDEN_TEST)
    add_test(NAME demo_transcript
             COMMAND ${CMAKE_COMMAND}
                 -DEXE=$<TARGET_FILE:terminal_romanian_whist>
                 -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/golden/demo-seed-42.txt
                 -DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/demo-seed-42.txt
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/golden/CompareTranscript.cmake)

    set_tests_properties(demo_transcript PROPERTIES LABELS golden TIMEOUT 60)
endif()
