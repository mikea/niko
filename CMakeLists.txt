cmake_minimum_required(VERSION 3.27)

project(niko VERSION 0.1 DESCRIPTION "niko programming language" LANGUAGES C)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)

add_compile_options(
  -DLAPACK_ILP64
  -mavx2
  -std=gnu2x 
  -mmemset-strategy=vector_loop:-1:noalign
  -Wall -Werror=implicit-fallthrough -Werror=switch
  -Wno-psabi 
  -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer -fverbose-asm )
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-inline-small-functions -Og")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lexer.c
  COMMAND re2c -i --case-ranges -o ${CMAKE_CURRENT_BINARY_DIR}/lexer.c ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.re
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.re
  COMMENT "Generating lexer.c"
  VERBATIM
)

add_executable(niko 
  src/main.c src/words.c src/print.c src/blas.c
  ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)
target_link_libraries(niko m blas64 lapack64 lapacke64)


execute_process(
  COMMAND git describe --always --dirty
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(TIMESTAMP COMPILE_TIME UTC)
add_definitions(-DCOMPILE_TIME="${COMPILE_TIME}")
add_definitions(-DGIT_DESCRIBE="${GIT_DESCRIBE}")
