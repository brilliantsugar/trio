// This test is very tricky to compile:
//   1. Get llvm8 source code
//   2. Replace lib/Transforms/CDSPass/CDSPass.cpp with the one provided by c11tester
//   3. Run docker container with clang8, all other steps are done from the container but can be done without it if you have clang8 installed somewhere on your machine.
//   4. Compile libCDSPass.so in llvm with clang-8
//   5. Compile libModel from c11tester repo with clang-8 in the container
//   6. Compile this test like this: clang++-8 -Xclang -load -Xclang 3rdparty/llvm-8.0.1.src/build/lib/libCDSPass.so -L3rdparty/c11tester/ -lmodel -Wno-unused-command-line-argument -I.. -I3rdparty/c11tester/include -g -O3 -rdynamic test_c11tester.cc -o out/test_c11tester
//   7. Run this test: LD_LIBRARY_PATH=3rdparty/c11tester C11TESTER="-v3 -x1000" ./out/test_c11tester

#include <trio.h>
#include "model-assert.h"

#include <cassert>
#include <thread>

namespace {
    static constexpr size_t kLastNumber = 25;
    bs::Trio<int> trio{};
}

void producer() {
    for (int i = 1; i <= kLastNumber; ++i) {
        trio.Write() = i;
        trio.Commit();
    }
}

int main() {
    auto producer_thread = std::thread(producer);
    int prev_read = 0;
    while (prev_read != kLastNumber) {
        const auto& [val, changed] = trio.Read();
        (void)changed;
        MODEL_ASSERT(val >= prev_read);
        prev_read = val;
    }
    producer_thread.join();
    return 0;
}
