# Run after configuration:
# cmake -DBUILD_DIR=<build> -P tests/verify_ctest_environment.cmake
# Uses CMake/CTest only; does not require working Qt/PDFium executables.
cmake_minimum_required(VERSION 3.24)
if(NOT DEFINED BUILD_DIR)
    message(FATAL_ERROR "BUILD_DIR is required")
endif()
get_filename_component(BUILD_DIR "${BUILD_DIR}" ABSOLUTE)
get_filename_component(cmake_bin "${CMAKE_COMMAND}" DIRECTORY)
find_program(ctest NAMES ctest HINTS "${cmake_bin}" NO_DEFAULT_PATH REQUIRED)
execute_process(COMMAND "${ctest}" --test-dir "${BUILD_DIR}" --show-only=json-v1
    RESULT_VARIABLE result OUTPUT_VARIABLE discovery ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "CTest discovery failed: ${error}")
endif()
string(JSON count LENGTH "${discovery}" tests)
if(NOT count EQUAL 6)
    message(FATAL_ERROR "Expected six regression suites, got ${count}")
endif()
foreach(index RANGE 0 5)
    string(JSON name GET "${discovery}" tests ${index} name)
    string(JSON properties GET "${discovery}" tests ${index} properties)
    string(JSON property_count LENGTH "${properties}")
    math(EXPR last "${property_count} - 1")
    set(found FALSE)
    foreach(p RANGE 0 ${last})
        string(JSON key GET "${properties}" ${p} name)
        if(key STREQUAL "ENVIRONMENT")
            message(FATAL_ERROR "${name}: PATH must not be serialized as ENVIRONMENT")
        elseif(key STREQUAL "ENVIRONMENT_MODIFICATION")
            string(JSON modification GET "${properties}" ${p} value 0)
            if(NOT modification STREQUAL "PATH=path_list_prepend:${BUILD_DIR}/bin")
                message(FATAL_ERROR "${name}: unexpected PATH modification: ${modification}")
            endif()
            set(found TRUE)
        endif()
    endforeach()
    if(NOT found)
        message(FATAL_ERROR "${name}: missing runtime PATH modification")
    endif()
endforeach()

# Exercise the actual CTest modification with a multi-entry inherited PATH.
set(probe "${BUILD_DIR}/tests/environment-probe")
file(MAKE_DIRECTORY "${probe}")
set(expected_path "${BUILD_DIR}/bin;$ENV{PATH}")
file(WRITE "${probe}/check.cmake"
    "if(NOT \"\$ENV{PATH}\" STREQUAL [==[${expected_path}]==])\n"
    "  message(FATAL_ERROR \"CTest did not preserve inherited PATH\")\nendif()\n")
file(WRITE "${probe}/CTestTestfile.cmake"
    "add_test(environment_probe [==[${CMAKE_COMMAND}]==] -P [==[${probe}/check.cmake]==])\n"
    "set_tests_properties(environment_probe PROPERTIES ENVIRONMENT_MODIFICATION [==[${modification}]==])\n")
execute_process(COMMAND "${ctest}" --test-dir "${probe}" --output-on-failure
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "PATH inheritance probe failed:\n${output}\n${error}")
endif()
message(STATUS "All six suites use runtime PATH modification; inheritance probe passed")
