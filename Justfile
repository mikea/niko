alias w := watch

COMMON_CFLAGS := "-std=gnu2x -Wall -Werror=implicit-fallthrough -Werror=switch -I . -lm -g -msse2 -fno-math-errno -fno-trapping-math "
DEBUG_CFLAGS := COMMON_CFLAGS + "-fno-inline-small-functions -Og"
RELEASE_CFLAGS := COMMON_CFLAGS + "-O3"

watch +WATCH_TARGET='build':
    watchexec -rc -w . -- just {{WATCH_TARGET}}

build: (_build DEBUG_CFLAGS)
release: (_build RELEASE_CFLAGS)

run: build _test
    bin/niko

clean:
    rm -rf bin build

valgrind:
    valgrind --leak-check=full --track-origins=yes --show-reachable=yes bin/niko -t test_suite -v

benchmarks: release
    just _benchmarks > benchmarks.results

[private]
_test:
    bin/niko -t test_suite

[private]
_build CFLAGS:
    mkdir -p bin build
    re2c lexer.c -o build/lexer.c -i --case-ranges
    gcc {{CFLAGS}} -o bin/niko main.c build/lexer.c print.c words.c

_benchmarks:
    while IFS= read -r line; do echo "> $line"; hyperfine --warmup 10 --show-output "bin/niko -e \"$line\"" ; done < benchmarks
