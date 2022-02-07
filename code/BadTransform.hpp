#ifndef TASK_QUEUE_LECTURE_BADTRANSFORM_HPP
#define TASK_QUEUE_LECTURE_BADTRANSFORM_HPP

#include <algorithm>
#include <iterator>

#include "BadTaskQueue.hpp"

template < typename IterIn, typename IterOut, typename F >
void badParallelTransform(IterIn in_begin, IterIn in_end, IterOut out_begin, F&& transform_fun)
{
    const auto pool_size  = BadTaskQueue::pool_size();
    const auto n_elems    = std::distance(in_begin, in_end);
    const auto chunk_size = n_elems / pool_size;
    const auto remainder  = n_elems - chunk_size * pool_size;

    const auto process_chunk = [&](IterIn chunk_begin, IterIn chunk_end, IterOut out_it) {
        std::transform(chunk_begin, chunk_end, out_it, transform_fun);
    };

    for (size_t i = 0; i < pool_size - 1; ++i)
    {
        const auto current_chunk_size = chunk_size + static_cast< size_t >(i < remainder);
        BadTaskQueue::push(process_chunk, in_begin, in_begin + current_chunk_size, out_begin);
        in_begin += current_chunk_size;
        out_begin += current_chunk_size;
    }
    const auto current_chunk_size = chunk_size + static_cast< size_t >(pool_size - 1 < remainder);
    process_chunk(in_begin, in_begin + current_chunk_size, out_begin);
    BadTaskQueue::complete();
}

#endif // TASK_QUEUE_LECTURE_BADTRANSFORM_HPP
