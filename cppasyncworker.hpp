#pragma once

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <type_traits>

namespace cppasyncworker
{

    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;

    /**
     * @brief Returns the library version as a string.
     * @return The version string in "MAJOR.MINOR.PATCH" format.
     */
    inline std::string version()
    {
        return std::to_string(VERSION_MAJOR) + "." +
               std::to_string(VERSION_MINOR) + "." +
               std::to_string(VERSION_PATCH);
    }

    /**
     * @brief An asynchronous worker pool for executing tasks concurrently.
     */
    class WorkerPool
    {
    private:
        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;

        std::mutex queue_mutex_;
        std::condition_variable cv_task_;
        bool stop_ = false;

    public:
        /**
         * @brief Constructs a new WorkerPool.
         * @param threads The number of worker threads to spawn. Defaults to hardware concurrency.
         */
        explicit WorkerPool(size_t threads = std::thread::hardware_concurrency())
        {
            for (size_t i = 0; i < threads; ++i)
            {
                workers_.emplace_back([this]
                                      {
                    for (;;) {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex_);
                            
                            this->cv_task_.wait(lock, [this] {
                                return this->stop_ || !this->tasks_.empty();
                            });

                            if (this->stop_ && this->tasks_.empty()) {
                                return;
                            }

                            task = std::move(this->tasks_.front());
                            this->tasks_.pop();
                        }

                        task();
                    } });
            }
        }

        /**
         * @brief Destructor that joins all worker threads.
         */
        ~WorkerPool()
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                stop_ = true;
            }

            cv_task_.notify_all();

            for (std::thread &worker : workers_)
            {
                if (worker.joinable())
                {
                    worker.join();
                }
            }
        }

        // Prevent copying and assignment semantics
        WorkerPool(const WorkerPool &) = delete;
        WorkerPool &operator=(const WorkerPool &) = delete;
        WorkerPool(WorkerPool &&) = delete;
        WorkerPool &operator=(WorkerPool &&) = delete;

        /**
         * @brief Enqueues a task into the worker pool.
         * @tparam F The function type.
         * @tparam Args The argument types.
         * @param f The function to execute.
         * @param args The arguments to pass to the function.
         * @return A std::future containing the return value of the function.
         */
        template <class F, class... Args>
        [[nodiscard]] auto Enqueue(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args...>>
        {
            using return_type = std::invoke_result_t<F, Args...>;

            auto task = std::make_shared<std::packaged_task<return_type()>>(
                [func = std::forward<F>(f), args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable
                {
                    return std::apply(std::move(func), std::move(args_tuple));
                });

            std::future<return_type> res = task->get_future();

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                if (stop_)
                {
                    throw std::runtime_error("Enqueue called on stopped WorkerPool");
                }
                tasks_.emplace([task]()
                               { (*task)(); });
            }

            cv_task_.notify_one();
            return res;
        }
    };

} // namespace cppasyncworker