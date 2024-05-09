cmake_minimum_required(VERSION 3.27)

project(niko VERSION 0.1 DESCRIPTION "niko programming language" LANGUAGES C)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_compile_options(-std=gnu2x -Wall -Werror=implicit-fallthrough -Werror=switch -msse2 -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-inline-small-functions -Og")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g -mavx2")


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lexer.c
  COMMAND re2c -i --case-ranges -o ${CMAKE_CURRENT_BINARY_DIR}/lexer.c ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.re
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.re
  COMMENT "Generating lexer.c"
  VERBATIM
)

add_executable(niko src/main.c src/words.c src/print.c ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)
target_link_libraries(niko m)
