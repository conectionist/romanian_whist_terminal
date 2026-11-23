# Plays the seeded demo and compares what it prints with the recorded
# transcript. CTest runs this with EXE, EXPECTED and ACTUAL defined.

# The output depends on the environment, so the test sets it rather than
# inheriting the developer's: the locale picks the suit glyphs, the height
# picks the layout, and the WHIST_ variables override flags.
foreach(name LINES COLUMNS NO_COLOR TERM LANG LC_CTYPE WHIST_DELAY_MS WHIST_ASCII)
    unset(ENV{${name}})
endforeach()

set(ENV{LC_ALL} C)

# Piped output falls back to 24 rows, which is the compact layout. 30 rows is
# the full layout most players see.
set(ENV{LINES} 30)

execute_process(
    COMMAND "${EXE}" --demo --no-animate --seed=42 --ascii
    INPUT_FILE /dev/null
    OUTPUT_FILE "${ACTUAL}"
    ERROR_VARIABLE errors
    RESULT_VARIABLE result)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "The demo exited with ${result}:\n${errors}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${EXPECTED}" "${ACTUAL}"
    RESULT_VARIABLE different)

if(different)
    # Straight to the test's output: CMake rewraps the text of an error
    # message, which would scramble the diff.
    execute_process(
        COMMAND diff -u "${EXPECTED}" "${ACTUAL}"
        COMMAND head -n 60)

    message(FATAL_ERROR
        "The demo's transcript has changed; the first lines of the difference are above. "
        "If the change is intended, for example after an engine update or a change to the "
        "screen, accept it in the same pull request with:\n\n"
        "    cp ${ACTUAL} ${EXPECTED}\n")
endif()
