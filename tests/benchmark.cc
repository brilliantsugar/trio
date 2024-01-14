#include <trio.h>
#include <benchmark/benchmark.h>
#include <iostream>

class TrioFixture : public benchmark::Fixture {
protected:
  bs::Trio<int> trio{0};
};

BENCHMARK_DEFINE_F(TrioFixture, Write)(benchmark::State& st) {
    int num_swaps = st.range(0);
    int j = 0;
    for (auto _ : st) {
        for (int i = 0; i < num_swaps; ++i) {
            benchmark::DoNotOptimize(trio.Write() = j++);
            trio.Commit();
        }
    }
    st.SetItemsProcessed(st.iterations() * num_swaps);
}

BENCHMARK_DEFINE_F(TrioFixture, ReadStale)(benchmark::State& st) {
    int num_swaps = st.range(0);
    for (auto _ : st) {
        for (int i = 0; i < num_swaps; ++i) {
            benchmark::DoNotOptimize(trio.Read());
        }
    }
    st.SetItemsProcessed(st.iterations() * num_swaps);
}

BENCHMARK_DEFINE_F(TrioFixture, ReadWriteSingleThreaded)(benchmark::State& st) {
    int num_swaps = st.range(0);
    int j = 0;
    for (auto _ : st) {
        for (int i = 0; i < num_swaps; ++i) {
            benchmark::DoNotOptimize(trio.Write() = j++);
            trio.Commit();
            benchmark::DoNotOptimize(trio.Read());
        }
    }
    st.SetItemsProcessed(st.iterations() * num_swaps);
}

BENCHMARK_DEFINE_F(TrioFixture, ReadWriteMultiThreaded)(benchmark::State& st) {
    int num_swaps = st.range(0);
    if (st.thread_index() == 0) { // producer
        for (auto _ : st) {
            for (int i = 0; i < num_swaps; ++i) {
                trio.Write() = i;
                trio.Commit();
            }
        }
    } else { // consumer
        size_t total_reads = 0;
        size_t updates = 0;
        for (auto _ : st) {
            for (int i = 0; i < num_swaps; ++i) {
                auto [data, has_changed] = trio.Read();
                ++total_reads;
                int pr = updates;
                updates += static_cast<unsigned>(has_changed);
            }
        }

        st.counters["new_to_stale_ratio"] = updates / static_cast<double>(total_reads + 1);
    }
    st.SetItemsProcessed(st.iterations() * num_swaps);
}

BENCHMARK_REGISTER_F(TrioFixture, Write)
    ->Threads(1)
    ->Unit(benchmark::kMillisecond)
    ->Range(50000, 5000000)
    ->UseRealTime();

BENCHMARK_REGISTER_F(TrioFixture, ReadStale)
    ->Threads(1)
    ->Unit(benchmark::kMillisecond)
    ->Range(50000, 5000000)
    ->UseRealTime();

BENCHMARK_REGISTER_F(TrioFixture, ReadWriteSingleThreaded)
    ->Threads(1)
    ->Unit(benchmark::kMillisecond)
    ->Range(50000, 5000000)
    ->UseRealTime();

BENCHMARK_REGISTER_F(TrioFixture, ReadWriteMultiThreaded)
    ->Threads(2)
    ->Unit(benchmark::kMillisecond)
    ->Range(50000, 5000000)
    ->UseRealTime();

BENCHMARK_MAIN();
