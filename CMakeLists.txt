cmake_minimum_required(VERSION 3.16)
project(bfpp_c_oop VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BF_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(BF_LIB_SOURCES
    src/core/common.c
    src/core/object.c
    src/io/io_base.c
    src/io/io_stdio.c
    src/io/io_buffer.c
    src/tape/tape_base.c
    src/tape/tape_fixed.c
    src/tape/tape_grow.c
    src/program/program.c
    src/compiler/compiler_base.c
    src/compiler/compiler_classic.c
    src/compiler/compiler_bfpp.c
    src/compiler/compiler_core.c
    src/vm/vm_switch.c
)

add_library(bfcore STATIC ${BF_LIB_SOURCES})
set(BF_PRIVATE_INCLUDE_DIRS
    src/core src/compiler src/io src/tape src/program src/vm
)
target_include_directories(bfcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/bf)
target_include_directories(bfcore PRIVATE ${BF_PRIVATE_INCLUDE_DIRS})

target_compile_options(bfcore PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_executable(bfpp src/main.c)
target_include_directories(bfpp PRIVATE src)
target_link_libraries(bfpp PRIVATE bfcore)
target_compile_options(bfpp PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

enable_testing()
set(BF_TEST_TARGETS)
foreach(module compiler tape io object program vm)
    set(target test_${module})
    add_executable(${target} tests/unit/test_${module}.c tests/support/test.c)
    target_link_libraries(${target} PRIVATE bfcore)
    target_include_directories(${target} PRIVATE
        tests/support tests/unit ${BF_PRIVATE_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE
        TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    add_test(NAME unit_${module} COMMAND ${target})
    list(APPEND BF_TEST_TARGETS ${target})
endforeach()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME bfpp_runner_tests
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_bfpp_runner.py)
add_test(NAME cli_tests
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.py
        $<TARGET_FILE:bfpp>)
file(GLOB BFPP_TEST_CASES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/*.bfpp)
foreach(test_case IN LISTS BFPP_TEST_CASES)
    get_filename_component(test_name ${test_case} NAME_WE)
    add_test(NAME bfpp_${test_name}
        COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_bfpp_tests.py
            --file ${test_case} -- $<TARGET_FILE:bfpp>)
endforeach()

if(BF_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target bfcore bfpp ${BF_TEST_TARGETS})
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
endif()
