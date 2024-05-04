alias w := watch

COMMON_CFLAGS := "-std=gnu2x -Wall -Werror=implicit-fallthrough -Werror=switch -I . "
DEBUG_CFLAGS := COMMON_CFLAGS + "-g -fno-inline-small-functions -Og"
RELEASE_CFLAGS := COMMON_CFLAGS + "-O3"

watch +WATCH_TARGET='build':
    watchexec -rc -w . -- just {{WATCH_TARGET}}

build: (_build DEBUG_CFLAGS)
release: (_build RELEASE_CFLAGS)

run: build _test
    bin/farr

clean:
    rm -rf bin build

valgrind:
    valgrind --leak-check=full --track-origins=yes --show-reachable=yes bin/farr -t test_suite -v

[private]
_test:
    bin/farr -t test_suite

[private]
_build CFLAGS:
    mkdir -p bin build
    re2c lexer.c -o build/lexer.c -i --case-ranges
    gcc {{CFLAGS}} -o bin/farr main.c build/lexer.c print.c words.c
