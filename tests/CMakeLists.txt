include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(terminal_tests
    HandLayoutTests.cpp
    AppOptionsTests.cpp
    CardFormatTests.cpp
)

target_link_libraries(terminal_tests PRIVATE
    terminal_core
    Catch2::Catch2WithMain
)

target_include_directories(terminal_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(terminal_tests PRIVATE cxx_std_20)

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
