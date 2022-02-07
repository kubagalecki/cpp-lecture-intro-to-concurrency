#ifndef TASK_QUEUE_LECTURE_BADTASKQUEUE_HPP
#define TASK_QUEUE_LECTURE_BADTASKQUEUE_HPP

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

class BadTaskQueue
{
    using task_t = std::function< void() >;
    std::deque< task_t >        task_queue;
    std::mutex                  task_mutex;
    std::condition_variable     task_cv, complete_cv;
    std::atomic_size_t          n_busy{0};
    std::atomic_bool            stop_flag{false};
    std::vector< std::jthread > threads;

    task_t try_pop()
    {
        task_t          task{};
        std::lock_guard lock{task_mutex};
        if (!task_queue.empty())
        {
            task = std::move(task_queue.front());
            task_queue.pop_front();
            ++n_busy;
        }
        return task;
    }

    std::jthread make_runner()
    {
        return std::jthread{[this] {
            while (!stop_flag)
            {
                task_t task = try_pop();
                if (task)
                {
                    task();
                    --n_busy;
                }
                else
                {
                    std::unique_lock lock{task_mutex};
                    if (task_queue.empty() && n_busy == 0)
                        complete_cv.notify_all();
                    task_cv.wait(lock, [this] { return !task_queue.empty() || stop_flag; });
                }
            }
        }};
    }

    template < typename F, typename... Args >
    void push_task(F&& fun, Args&&... args)
    {
        {
            std::unique_lock lock{task_mutex};
            task_queue.emplace_back(
                [f = std::forward< F >(fun), ... a = std::forward< Args >(args)]() mutable { f(std::move(a)...); });
        }
        task_cv.notify_one();
    }

    void complete_tasks()
    {
        const auto check_complete = [this] {
            return task_queue.empty() && n_busy == 0;
        };
        std::unique_lock lock{task_mutex};
        if (check_complete())
            return;
        complete_cv.wait(lock, check_complete);
    }

    BadTaskQueue()
    {
        const auto hw_conc = std::thread::hardware_concurrency();
        threads.reserve(hw_conc);
        std::generate_n(std::back_inserter(threads), hw_conc, [this] { return make_runner(); });
    }

    static BadTaskQueue instance;

public:
    BadTaskQueue(const BadTaskQueue&) = delete;
    BadTaskQueue(BadTaskQueue&&)      = delete;
    BadTaskQueue& operator=(const BadTaskQueue&) = delete;
    BadTaskQueue& operator=(BadTaskQueue&&) = delete;
    ~BadTaskQueue()
    {
        stop_flag = true;
        task_cv.notify_all();
    }

    template < typename F, typename... Args >
    static void push(F&& fun, Args&&... args)
    {
        instance.push_task(std::forward< F >(fun), std::forward< Args >(args)...);
    }

    static void complete() { instance.complete_tasks(); }

    static size_t pool_size() { return std::thread::hardware_concurrency(); }
};

BadTaskQueue BadTaskQueue::instance{};

#endif // TASK_QUEUE_LECTURE_BADTASKQUEUE_HPP
