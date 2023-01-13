#ifndef TASK_QUEUE_LECTURE_BADASYNCTASKQUEUE_HPP
#define TASK_QUEUE_LECTURE_BADASYNCTASKQUEUE_HPP

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class BadAsyncTaskQueue
{
    using task_t = std::move_only_function< void() >;
    auto makeRunner() -> std::jthread
    {
        return std::jthread{[this](std::stop_token stop_token) {
            while (!stop_token.stop_requested())
            {
                task_t my_task;
                {
                    std::unique_lock lock{queue_mutex_};
                    cond_var_.wait(lock, stop_token, [&] { return !task_queue_.empty(); });
                    if (!task_queue_.empty())
                    {
                        my_task = std::move(task_queue_.front());
                        task_queue_.pop();
                    }
                }
                if (my_task)
                    std::invoke(my_task);
            }
        }};
    }

public:
    explicit BadAsyncTaskQueue(unsigned int num_threads = std::thread::hardware_concurrency())
    {
        thread_pool_.reserve(num_threads);
        std::generate_n(std::back_inserter(thread_pool_), num_threads, [this] { return makeRunner(); });
    }
    [[nodiscard]] auto numWorkers() const { return thread_pool_.size(); }

    template < typename F, typename... Args >
    auto submitForExecution(F&& fun, Args&&... args)
    {
        auto bound_task       = std::bind_front(std::forward< F >(fun), std::forward< Args >(args)...);
        auto packaged_task    = std::packaged_task{[task = std::move(bound_task)] mutable {
            return std::invoke(task);
        }};
        auto future           = packaged_task.get_future();
        auto type_erased_task = task_t{[task = std::move(packaged_task)] mutable {
            std::invoke(task);
        }};
        {
            std::lock_guard lock{queue_mutex_};
            task_queue_.push(std::move(type_erased_task));
        }
        cond_var_.notify_one();
        return future;
    }

private:
    std::queue< task_t >        task_queue_;
    std::mutex                  queue_mutex_;
    std::condition_variable_any cond_var_;
    std::vector< std::jthread > thread_pool_;
};
#endif // TASK_QUEUE_LECTURE_BADASYNCTASKQUEUE_HPP
