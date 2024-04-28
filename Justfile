alias w := watch

CFLAGS := "-std=gnu2x -Wall -Werror=switch -I . -g"

watch +WATCH_TARGET='build':
    watchexec -rc -w . -- just {{WATCH_TARGET}}

build:
    mkdir -p bin build
    re2c lexer.c -o build/lexer.c -i --case-ranges
    gcc {{CFLAGS}} -o bin/farr main.c build/lexer.c

run: build
    bin/farr

clean:
    rm -rf bin build