cmake_minimum_required(VERSION 3.27)

project(niko VERSION 0.1 DESCRIPTION "niko programming language" LANGUAGES C CXX)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
add_definitions(-D_GNU_SOURCE)

set (CMAKE_CXX_STANDARD 23)

add_compile_options(
  -mavx2
  -Wall
  -Werror=implicit-fallthrough -Werror=switch -Werror=return-type
  # todo
  -Wno-pointer-arith
  -fno-math-errno -fno-trapping-math 
  -fno-omit-frame-pointer -fverbose-asm)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-inline-small-functions -Og")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -Wno-unknown-pragmas -Werror")
add_link_options(-z execstack)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# code coverage
if (DEFINED ENV{COVERAGE})
  message("Enabling code coverage")
  add_compile_options(--coverage -ftest-coverage)
  add_link_options(--coverage)
endif()

add_executable(niko 
  src/main.c++ src/inter.c++ src/array.c++ src/words.c++ src/print.c++
  ${CMAKE_CURRENT_BINARY_DIR}/lexer.c++
  ${CMAKE_CURRENT_BINARY_DIR}/prelude.h)
target_link_libraries(niko m)
target_link_libraries(niko jemalloc)

# configure timestamp
execute_process(
  COMMAND git describe --always --dirty
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(TIMESTAMP COMPILE_TIME UTC)
add_definitions(-DCOMPILE_TIME="${COMPILE_TIME}")
add_definitions(-DGIT_DESCRIBE="${GIT_DESCRIBE}")
add_definitions(-DBUILD_TYPE="${CMAKE_BUILD_TYPE}")

# generate lexer
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lexer.c++
  COMMAND re2c -i --case-ranges -o ${CMAKE_CURRENT_BINARY_DIR}/lexer.c++ ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.c++
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
