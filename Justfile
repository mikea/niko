alias w := watch

COMMON_CFLAGS := "-std=gnu2x -Wall -Werror=implicit-fallthrough -Werror=switch -I . -lm -g -msse2 -fno-math-errno -fno-trapping-math "
DEBUG_CFLAGS := COMMON_CFLAGS + "-fno-inline-small-functions -Og"
# useful flags: -Winline 
RELEASE_CFLAGS := COMMON_CFLAGS + "-O3 -DNDEBUG -fno-omit-frame-pointer -march=native"

watch +WATCH_TARGET='build':
    watchexec -rc -w . -- just {{WATCH_TARGET}}

build: (_build DEBUG_CFLAGS)
release: (_build RELEASE_CFLAGS)

run: test
    bin/niko

test: build _test

clean:
    rm -rf bin build

valgrind:
    valgrind --leak-check=full --track-origins=yes --show-reachable=yes bin/niko -t test_suite -v

callgrind: release
    rm callgrind.out.*
    valgrind --tool=callgrind --dump-instr=yes bin/niko -e "1000000 ones 1000000 ones +"

benchmarks: release
    just _benchmarks > benchmarks.results

opt-report:
    cpp -DNDEBUG -I . -P words.c > build/words.c
    clang-format -i build/words.c
    gcc {{RELEASE_CFLAGS}} -fopt-info-vec-missed -S -o build/words.o build/words.c

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
