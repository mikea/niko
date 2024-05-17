alias w := watch

watch +WATCH_TARGET='test':
    watchexec -rc -w . --ignore *.results -- just {{WATCH_TARGET}}

build BUILD_TYPE="Debug":
    mkdir -p bin build/{{BUILD_TYPE}}
    rm -rf bin/niko
    cmake -B build/{{BUILD_TYPE}} -DCMAKE_BUILD_TYPE={{BUILD_TYPE}} -G Ninja
    cmake --build build/{{BUILD_TYPE}}
    cp build/{{BUILD_TYPE}}/niko bin/niko

release: (build "Release") _test

run: test
    rlwrap bin/niko

test: build _test

clean:
    rm -rf bin build callgrind.out.* perf.data perf.data.old vgcore.*

valgrind: build
    valgrind --leak-check=full --track-origins=yes --show-reachable=yes --suppressions=default.supp bin/niko -t tests/core.md -v
    valgrind --leak-check=full --track-origins=yes --show-reachable=yes --suppressions=default.supp bin/niko -t tests/prelude.md -v

valgrind-expr EXPR="10000000 zeros": build
    valgrind --leak-check=full --track-origins=yes --show-reachable=yes  --suppressions=default.supp bin/niko -e "{{EXPR}}"

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

[private]
_test:
    bin/niko -z -t tests/inter.md
    bin/niko -z -t tests/core.md
    bin/niko -t tests/prelude.md
    bin/niko -t docs/examples.md
    bin/niko -t docs/reference.md

[private]
_benchmarks:
    while IFS= read -r line; do echo "> $line"; \
        perf stat -- build/Release/niko -e "$line" 2>&1 ; \
        hyperfine --warmup 10 "build/Release/niko -e \"$line\"" ; \
        done < benchmarks
