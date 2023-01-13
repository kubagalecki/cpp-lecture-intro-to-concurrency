#include <execution>
#include <iostream>
#include <random>
#include <string>

#include "BadAsyncTaskQueue.hpp"
#include "BadWorkerPool.hpp"

#include "benchmark/benchmark.h"

std::vector< std::string > makeInput(size_t str_len, size_t num_strs)
{
    static std::mt19937        prng{std::random_device{}()};
    std::vector< std::string > ret{};
    ret.reserve(num_strs);
    std::generate_n(std::back_inserter(ret), num_strs, [&]() {
        std::string s;
        s.reserve(str_len);
        std::generate_n(std::back_inserter(s), str_len, [&] -> char {
            return std::uniform_int_distribution< short >{0, 127}(prng);
        });
        return s;
    });
    return ret;
}

auto prepInputOutput(const benchmark::State& state)
{
    auto inputs = makeInput(state.range(0), state.range(1));
    return std::make_pair(std::move(inputs), std::vector< size_t >(inputs.size()));
}

void setCounter(benchmark::State& state)
{
    state.counters["Hash Rate"] = benchmark::Counter{
        static_cast< double >(state.iterations() * state.range(0) * state.range(1)), benchmark::Counter::kIsRate};
}

static void Baseline(benchmark::State& state)
{
    auto init             = prepInputOutput(state);
    auto& [input, output] = init;
    for (auto _ : state)
    {
        std::transform(input.cbegin(), input.cend(), output.begin(), std::hash< std::string >{});
    }
    setCounter(state);
}

static void BM_BadAsync(benchmark::State& state)
{
    auto init             = prepInputOutput(state);
    auto& [input, output] = init;
    BadWorkerPool ba;
    for (auto _ : state)
    {
        for (size_t i = 0; i < input.size(); ++i)
            ba.submitForExecution([&](size_t ind) { output[ind] = std::hash< std::string >{}(input[ind]); }, i);
        ba.complete();
    }
    setCounter(state);
}

static void BM_BadAsyncTransform(benchmark::State& state)
{
    auto init             = prepInputOutput(state);
    auto& [input, output] = init;
    for (auto _ : state)
    {
        badLaunchParallelTransformOnPool(input.cbegin(), input.cend(), output.begin(), std::hash< std::string >{});
    }
    setCounter(state);
}

static void BM_BadAsyncTransformOnQueue(benchmark::State& state)
{
    auto [input, output] = prepInputOutput(state);
    BadAsyncTaskQueue                    queue;
    std::vector< std::future< size_t > > futures(input.size());
    for (auto _ : state)
    {
        std::transform(input.cbegin(), input.cend(), futures.begin(), [&queue](std::string_view text) {
            return queue.submitForExecution(std::hash< std::string_view >{}, text);
        });
        for (size_t i = 0; auto& f : futures)
            output[i++] = f.get();
    }
    setCounter(state);
}

static void BM_BadAsyncTransformOnQueueChunked(benchmark::State& state)
{
    auto [input, output] = prepInputOutput(state);
    BadAsyncTaskQueue                  queue;
    const auto                         num_workers = queue.numWorkers();
    std::vector< std::future< void > > futures(num_workers);

    const auto target_chunk_size =
        input.size() % num_workers == 0 ? input.size() / num_workers : (input.size() / num_workers + 1);
    const auto hash_chunk = [&](size_t begin, size_t end) {
        std::transform(std::next(input.cbegin(), begin),
                       std::next(input.cbegin(), end),
                       std::next(output.begin(), begin),
                       std::hash< std::string >{});
    };
    for (auto _ : state)
    {
        for (size_t offset = 0, chunk_ind = 0; offset < input.size(); offset += target_chunk_size)
        {
            const auto end       = std::min(offset + target_chunk_size, input.size());
            futures[chunk_ind++] = queue.submitForExecution(hash_chunk, offset, end);
        }
        for (size_t i = 0; auto& f : futures)
            f.get();
    }
    setCounter(state);
}

static void BM_STLTransform(benchmark::State& state)
{
    auto init             = prepInputOutput(state);
    auto& [input, output] = init;
    for (auto _ : state)
    {
        std::transform(
            std::execution::par_unseq, input.cbegin(), input.cend(), output.begin(), std::hash< std::string >{});
    }
    setCounter(state);
}

#define COMMON_BM_PARAMS RangeMultiplier(2)->Ranges(param_range)->Unit(benchmark::kMillisecond)->UseRealTime()

static const std::vector< std::pair< std::int64_t, std::int64_t > > param_range = {{1 << 7, 1 << 12},
                                                                                   {1 << 7, 1 << 12}};
BENCHMARK(Baseline)->Name("Serial")->COMMON_BM_PARAMS;
BENCHMARK(BM_BadAsync)->Name("New_thread_per_string")->COMMON_BM_PARAMS;
BENCHMARK(BM_BadAsyncTransformOnQueue)->Name("New_task_per_string")->COMMON_BM_PARAMS;
BENCHMARK(BM_BadAsyncTransform)->Name("New_thread_per_chunk")->COMMON_BM_PARAMS;
BENCHMARK(BM_BadAsyncTransformOnQueueChunked)->Name("New_task_per_chunk")->COMMON_BM_PARAMS;
BENCHMARK(BM_STLTransform)->Name("STL_parallel_transform")->COMMON_BM_PARAMS;
BENCHMARK_MAIN();
