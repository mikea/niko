cmake_minimum_required(VERSION 3.27)

project(niko VERSION 0.1 DESCRIPTION "niko programming language" LANGUAGES C CXX)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
add_definitions(-D_GNU_SOURCE)

set (CMAKE_CXX_STANDARD 23)

add_compile_options(
  -march=native
  -Wall
  -Werror
  # todo
  -Wno-pointer-arith
  -fno-math-errno -fno-trapping-math 
  -fno-omit-frame-pointer -fverbose-asm)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-inline-small-functions -Og")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -Wno-unknown-pragmas")
add_link_options(-z execstack)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# code coverage: todo (doesn't work)
if (DEFINED ENV{COVERAGE})
  message("Enabling code coverage")
  add_compile_options(--coverage -ftest-coverage)
  add_link_options(--coverage)
endif()

add_executable(niko 
  src/main.c++ src/inter.c++ src/array.c++ src/words.c++ src/print.c++
  src/words/binop.c++ src/words/math.c++
  ${CMAKE_CURRENT_BINARY_DIR}/lexer.c++
  ${CMAKE_CURRENT_BINARY_DIR}/prelude.h
  ${CMAKE_CURRENT_BINARY_DIR}/timestamp.h)
target_link_libraries(niko m)
target_link_libraries(niko jemalloc)
add_dependencies(niko generate_timestamp)

# configure git info
execute_process(
  COMMAND git describe --always --dirty
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions(-DGIT_DESCRIBE="${GIT_DESCRIBE}")
add_definitions(-DBUILD_TYPE="${CMAKE_BUILD_TYPE}")

# Generate timestamp only when building, not during configure
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/timestamp.h
  COMMAND sh -c "echo '#define COMPILE_TIME \"'`date -u +%Y-%m-%dT%H:%M:%SZ`'\"' > ${CMAKE_CURRENT_BINARY_DIR}/timestamp.h"
  COMMENT "Generating timestamp header"
  VERBATIM
)
add_custom_target(generate_timestamp DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/timestamp.h)

# generate lexer
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lexer.c++
  COMMAND re2c -i --case-ranges -Wno-sentinel-in-midrule -o ${CMAKE_CURRENT_BINARY_DIR}/lexer.c++ ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.c++
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.c++
  COMMENT "Generating lexer.c++"
  VERBATIM
)

# embed prelude.nk
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/prelude.h
  COMMAND xxd -n __prelude_nk -i ${CMAKE_CURRENT_SOURCE_DIR}/src/prelude.nk > ${CMAKE_CURRENT_BINARY_DIR}/prelude.h
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/prelude.nk
)

# Find Catch2
find_package(Catch2 3 QUIET)
if(Catch2_FOUND)
  # Unit tests
  add_executable(unit_tests
    tests/test_lexer.cpp
    tests/test_array.cpp
    src/inter.c++ src/array.c++ src/words.c++ src/print.c++
    src/words/binop.c++ src/words/math.c++
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.c++
    ${CMAKE_CURRENT_BINARY_DIR}/prelude.h
    ${CMAKE_CURRENT_BINARY_DIR}/timestamp.h)
  target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain m jemalloc)
  add_dependencies(unit_tests generate_timestamp)
  
  # Enable CTest integration
  include(CTest)
  include(Catch)
  catch_discover_tests(unit_tests)
else()
  message(STATUS "Catch2 not found, unit tests will not be built")
endif()
