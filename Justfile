alias w := watch

watch +WATCH_TARGET='build':
    watchexec -rc -w . -- just {{WATCH_TARGET}}

build:
    mkdir -p bin build
    re2c main.c -o build/main.c -i --case-ranges
    gcc -std=c2x -o bin/farr build/main.c -Wall -I .

run: build
    bin/farr

clean:
    rm -rf bin build