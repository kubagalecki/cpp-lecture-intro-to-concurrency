#ifndef TASK_QUEUE_LECTURE_BADWORKERPOOL_HPP
#define TASK_QUEUE_LECTURE_BADWORKERPOOL_HPP

#include <algorithm>
#include <thread>
#include <utility>
#include <vector>

class BadWorkerPool
{
public:
    BadWorkerPool() { launched_threads.reserve(std::thread::hardware_concurrency()); }
    template < typename F, typename... Args >
    void push(F&& fun, Args&&... args)
    {
        launched_threads.emplace_back(std::forward< F >(fun), std::forward< Args >(args)...);
    }
    void complete() { launched_threads.clear(); }

private:
    std::vector< std::jthread > launched_threads;
};

template < typename IterIn, typename IterOut, typename F >
void badLaunchParallelTransformOnPool(IterIn in_begin, IterIn in_end, IterOut out_begin, const F& transform_fun)
{
    const auto pool_size  = std::thread::hardware_concurrency();
    const auto n_elems    = std::distance(in_begin, in_end);
    const auto chunk_size = n_elems / pool_size;
    const auto remainder  = n_elems - chunk_size * pool_size;

    const auto process_chunk = [&](IterIn chunk_begin, IterIn chunk_end, IterOut out_it) {
        std::transform(chunk_begin, chunk_end, out_it, transform_fun);
    };

    BadWorkerPool ba;

    for (size_t i = 0; i < pool_size - 1; ++i)
    {
        const auto current_chunk_size = chunk_size + static_cast< size_t >(i < remainder);
        ba.push(process_chunk, in_begin, in_begin + current_chunk_size, out_begin);
        in_begin += current_chunk_size;
        out_begin += current_chunk_size;
    }
    const auto current_chunk_size = chunk_size + static_cast< size_t >(pool_size - 1 < remainder);
    process_chunk(in_begin, in_begin + current_chunk_size, out_begin);

    ba.complete();
}

#endif // TASK_QUEUE_LECTURE_BADWORKERPOOL_HPP
