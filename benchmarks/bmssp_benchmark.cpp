#include <benchmark/benchmark.h>
#include <vector>

static void BM_Dummy(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> v(1000, 0);
        benchmark::DoNotOptimize(v.data());
    }
}
BENCHMARK(BM_Dummy);

// שימי לב: אין BENCHMARK_MAIN() כי קישרנו ל benchmark_main בספריות
