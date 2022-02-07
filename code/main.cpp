#include <execution>
#include <iostream>
#include <random>
#include <string>

#include "BadTransform.hpp"
#include "BadWorkerPool.hpp"

#include "benchmark/benchmark.h"

inline constexpr size_t n_strings     = 1u << 10; // note:  1u << n == 2 to the power of n
inline constexpr size_t string_length = 1u << 12;

std::vector< std::string > makeInput()
{
    static std::mt19937        prng{std::random_device{}()};
    std::vector< std::string > ret{};
    ret.reserve(n_strings);
    std::generate_n(std::back_inserter(ret), n_strings, [&]() {
        std::string s;
        s.reserve(string_length);
        std::generate_n(
            std::back_inserter(s), string_length, [&] { return std::uniform_int_distribution< char >{}(prng); });
        return s;
    });
    return ret;
}

auto prepInputOutput()
{
    auto inputs = makeInput();
    return std::make_pair(std::move(inputs), std::vector< size_t >(inputs.size()));
}

static void Baseline(benchmark::State& state)
{
    auto init             = prepInputOutput();
    auto& [input, output] = init;
    for (auto _ : state)
    {
        std::transform(input.cbegin(), input.cend(), output.begin(), std::hash< std::string >{});
    }
    state.SetBytesProcessed(state.iterations() * string_length * n_strings);
}

static void BM_BadAsync(benchmark::State& state)
{
    auto init             = prepInputOutput();
    auto& [input, output] = init;
    BadWorkerPool ba;
    for (auto _ : state)
    {
        for (size_t i = 0; i < input.size(); ++i)
            ba.push([&](size_t ind) { output[ind] = std::hash< std::string >{}(input[ind]); }, i);
        ba.complete();
    }
    state.SetBytesProcessed(state.iterations() * string_length * n_strings);
}

static void BM_BadAsyncTransform(benchmark::State& state)
{
    auto init             = prepInputOutput();
    auto& [input, output] = init;
    for (auto _ : state)
    {
        badLaunchParallelTransformOnPool(input.cbegin(), input.cend(), output.begin(), std::hash< std::string >{});
    }
    state.SetBytesProcessed(state.iterations() * string_length * n_strings);
}

static void BM_BadTransform(benchmark::State& state)
{
    auto init             = prepInputOutput();
    auto& [input, output] = init;
    for (auto _ : state)
    {
        badParallelTransform(input.cbegin(), input.cend(), output.begin(), std::hash< std::string >{});
    }
    state.SetBytesProcessed(state.iterations() * string_length * n_strings);
}

static void BM_STLTransform(benchmark::State& state)
{
    auto init             = prepInputOutput();
    auto& [input, output] = init;
    for (auto _ : state)
    {
        std::transform(
            std::execution::par_unseq, input.cbegin(), input.cend(), output.begin(), std::hash< std::string >{});
    }
    state.SetBytesProcessed(state.iterations() * string_length * n_strings);
}

BENCHMARK(Baseline)->UseRealTime()->Name("Serial");
BENCHMARK(BM_BadAsync)->UseRealTime()->Name("New thread per string");
BENCHMARK(BM_BadAsyncTransform)->UseRealTime()->Name("Split among [n_cores] threads");
BENCHMARK(BM_BadTransform)->UseRealTime()->Name("Submit to task queue");
BENCHMARK(BM_STLTransform)->UseRealTime()->Name("STL parallel transform");
BENCHMARK_MAIN();
