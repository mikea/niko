alias w := watch

watch +WATCH_TARGET='build':
    watchexec -rc -w . --ignore *.results -- just {{WATCH_TARGET}}

build: (_build "Debug")
release: (_build "Release")

run: test
    bin/niko

test: build _test

clean:
    rm -rf bin build callgrind.out.* perf.data perf.data.old

valgrind:
    valgrind --leak-check=full --track-origins=yes --show-reachable=yes bin/niko -t test_suite -v

callgrind EXPR="10000000 zeros": release
    rm -f callgrind.out.* cachegrind.out.*
    # 
    valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes bin/niko -e "{{EXPR}}"

cachegrind EXPR="10000000 zeros": release
    rm -f callgrind.out.* cachegrind.out.*
    valgrind --tool=cachegrind bin/niko -e {{EXPR}}"

benchmarks: release
    taskset -c 4 just _benchmarks > benchmarks.results

stat EXPR="10000000 zeros": release
    perf stat -- bin/niko -e "{{EXPR}}"

# opt-report:
#     gcc {{RELEASE_CFLAGS}} -E -P -o build/words.c words.c
#     gcc {{RELEASE_CFLAGS}} -fverbose-asm -S -o build/words.s build/words.c
#     clang-format -i build/words.c
#     gcc {{RELEASE_CFLAGS}} -fopt-info-vec-missed -c -o build/words.o build/words.c

docs: build
    bin/niko -m docs/examples.md

[private]
_test:
    bin/niko -t test_suite

[private]
_build BUILD_TYPE:
    mkdir -p bin build/{{BUILD_TYPE}}
    rm -rf bin/niko
    cmake -B build/{{BUILD_TYPE}} -DCMAKE_BUILD_TYPE={{BUILD_TYPE}}
    cmake --build build/{{BUILD_TYPE}}
    cp build/{{BUILD_TYPE}}/niko bin/niko

_benchmarks:
    while IFS= read -r line; do echo "> $line"; \
        perf stat -- build/Release/niko -e "$line" 2>&1 ; \
        hyperfine --warmup 10 "build/Release/niko -e \"$line\"" ; \
        done < benchmarks
