file(MAKE_DIRECTORY "${WORK_DIR}")
execute_process(COMMAND "${PROGRAM}" --deterministic --count 8 --interval-ms 0
    --log-dir "${WORK_DIR}/logs" OUTPUT_FILE /dev/full ERROR_VARIABLE error
    RESULT_VARIABLE result TIMEOUT 5)
if(NOT result STREQUAL "1" OR NOT error MATCHES "^Error: [^\r\n]+[\r\n]*$")
    message(FATAL_ERROR "Expected reported stdout write failure and exit 1, got ${result}: ${error}")
endif()
message(STATUS "Buffered stdout write failure surfaced with exit 1")
