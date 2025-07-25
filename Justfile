set fallback
alias w := watch

deps:
    sudo apt update
    sudo apt install --no-install-recommends --no-upgrade -y gcc-14 just re2c valgrind ninja-build libjemalloc-dev xxd cmake ninja-build clang-format catch2

watch +WATCH_TARGET='test':
    watchexec --print-events -rc -w . --ignore *.results -- just {{WATCH_TARGET}}

build BUILD_TYPE="Debug":
    mkdir -p bin build/{{BUILD_TYPE}}
    rm -rf bin/niko
    CMAKE_EXPORT_COMPILE_COMMANDS=1 cmake -B build/{{BUILD_TYPE}} -DCMAKE_BUILD_TYPE={{BUILD_TYPE}} -G Ninja
    cmake --build build/{{BUILD_TYPE}}
    cp build/{{BUILD_TYPE}}/niko bin/niko
    @if [ "{{BUILD_TYPE}}" = "Debug" ]; then \
        ln -sf build/{{BUILD_TYPE}}/compile_commands.json compile_commands.json; \
    fi

release: (build "Release") (_test "Release")

run: test
    bin/niko

test: build (_test "Debug")

clean:
    rm -rf bin build callgrind.out.* perf.data perf.data.old vgcore.*

format:
    clang-format -i src/*.c++ src/*.h

coverage: clean
    COVERAGE=ON just test
    find build/Debug -name "*.gcda" | xargs gcov

valgrind-test FILE BUILD_TYPE="Debug": (build BUILD_TYPE) (_valgrind-test FILE)
valgrind BUILD_TYPE="Debug": (build BUILD_TYPE) (_valgrind-test "tests/inter.md") (_valgrind-test "tests/words.md") (_valgrind-test "tests/prelude.md")  (_valgrind-test "docs/examples.md") 

valgrind-expr EXPR="1000 .": build 
    valgrind --leak-check=full --track-origins=yes --show-reachable=yes  --suppressions=default.supp bin/niko -z -e "{{EXPR}}"

callgrind EXPR="10000000 zeros": release
    rm -f callgrind.out.* cachegrind.out.*
    valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes bin/niko -e "{{EXPR}}"

callgrind-test FILE: (build "Release")
    rm -f callgrind.out.* cachegrind.out.*
    valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes bin/niko -t "{{FILE}}"

cachegrind EXPR="10000000 zeros": release
    rm -f callgrind.out.* cachegrind.out.*
    valgrind --tool=cachegrind bin/niko -e "{{EXPR}}"

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
_test BUILD_TYPE="Debug":
    build/{{BUILD_TYPE}}/unit_tests
    bin/niko -z -t tests/inter.md
    bin/niko -z -t tests/words.md
    bin/niko -t tests/prelude.md
    bin/niko -t docs/examples.md
    bin/niko -t docs/manual.md
    bin/niko -t docs/reference.md
    bin/niko -t docs/example_test.md
    bin/niko -t docs/euler.md
    bin/niko -t docs/examples/rubik-2x2.md
    bin/niko -t docs/examples/h2o.md

[private]
_benchmarks:
    while IFS= read -r line; do \
        echo "" ; \
        echo "========================================" ; \
        echo "BENCHMARK: $line" ; \
        echo "========================================" ; \
        TEMP_FILE=$(mktemp --suffix=.nk) ; \
        echo "$line" > "$TEMP_FILE" ; \
        hyperfine --warmup 10 "build/Release/niko $TEMP_FILE" ; \
        echo "Cachegrind:" ; \
        valgrind --tool=cachegrind --cache-sim=yes --branch-sim=yes --cachegrind-out-file=/tmp/cachegrind.out build/Release/niko $TEMP_FILE 2>&1 | tail -n 25 | grep -E "==.*==" ; \
        rm "$TEMP_FILE" ; \
        rm -f /tmp/cachegrind.out ; \
        done < benchmarks

[private]
_valgrind-test FILE: 
    valgrind --leak-check=full --error-exitcode=121 --track-origins=yes --show-reachable=yes --suppressions=default.supp bin/niko -t {{FILE}} -v
