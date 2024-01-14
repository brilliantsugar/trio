#!/bin/bash

set -e

if [ ! -f "example.cc" ]; then
    echo "Should run under tests/ directory"
    exit 1
fi

CC=g++
OUT=out
BENCHMARK_BUILD=3rdparty/benchmark/build/src
CCFLAGS="-O3 -I../ -I3rdparty/benchmark/include -L${BENCHMARK_BUILD} -I3rdparty/relacy -std=c++20"
CCFLAGS_PEDANTIC="${CCFLAGS} -pedantic -Werror"

mkdir -p ${OUT}

echo "Building relacy test..."
# Relacy is old. Need to disable a bunch of warnings for it to compile on modern compilers
${CC} test_relacy.cc -o ${OUT}/test_relacy ${CCFLAGS} -DTRIO_USE_RELACY -Wno-deprecated -Wno-inline-new-delete
echo "Building example..."
${CC} example.cc -o ${OUT}/example ${CCFLAGS_PEDANTIC}
echo "Building benchmark..."
BENCHMARK_FILE_TEST=${BENCHMARK_BUILD}/libbenchmark.a
if [[ -f "$BENCHMARK_FILE_TEST" ]]
then
	${CC} ${CCFLAGS_PEDANTIC} benchmark.cc -o ${OUT}/benchmark -lbenchmark -lbenchmark_main
	${CC} ${CCFLAGS_PEDANTIC} benchmark.cc -o ${OUT}/benchmark_mutex -DTRIO_USE_MUTEX -lbenchmark -lbenchmark_main
else
    echo "${BENCHMARK_FILE_TEST} does not exist. Build google benchmark to be able to run benchmarks"
fi
echo "Done. All compiled binaries are in the ${OUT}/ directory"
