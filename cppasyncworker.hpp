// cpp-asyncworker - A lightweight, thread-safe, and header-only C++ library for managing asynchronous tasks using a worker pool
// https://github.com/jonoton/cpp-asyncworker
// Copyright (c) 2025 cpp-asyncworker
// Licensed under the MIT License. SPDX-License-Identifier: MIT

#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

namespace cppasyncworker {

constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 3;
constexpr int VERSION_PATCH = 0;

/**
 * @brief Returns the library version as a string.
 * @return The version string in "MAJOR.MINOR.PATCH" format.
 */
inline std::string version() {
  return std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) +
         "." + std::to_string(VERSION_PATCH);
}

/**
 * @brief A callback type for reporting unhandled exceptions.
 * @note The callback is invoked from worker threads and MUST be thread-safe.
 */
using ErrorHandler = std::function<void(const std::exception_ptr &)>;

/**
 * @brief A type-erased move-only callable wrapper.
 */
class UniqueTask {
 private:
  struct ImplBase {
    virtual ~ImplBase() = default;
    virtual void call() = 0;
  };

  template <typename F>
  struct Impl : ImplBase {
    F f;
    template <typename U>
    explicit Impl(U &&func) : f(std::forward<U>(func)) {}
    void call() override { f(); }
  };

  std::unique_ptr<ImplBase> impl_;

 public:
  UniqueTask() = default;

  template <typename F, typename = std::enable_if_t<
                            !std::is_same_v<std::decay_t<F>, UniqueTask>>>
  UniqueTask(F &&f)
      : impl_(std::make_unique<Impl<std::decay_t<F>>>(std::forward<F>(f))) {}

  ~UniqueTask() = default;

  UniqueTask(const UniqueTask &) = delete;
  UniqueTask &operator=(const UniqueTask &) = delete;

  UniqueTask(UniqueTask &&) noexcept = default;
  UniqueTask &operator=(UniqueTask &&) noexcept = default;

  void operator()() {
    if (!impl_) {
      throw std::bad_function_call();
    }
    impl_->call();
  }

  explicit operator bool() const noexcept { return static_cast<bool>(impl_); }
};

/**
 * @brief An asynchronous worker pool for executing tasks concurrently.
 */
class WorkerPool {
 private:
  // State shared with the worker threads. A worker that has to be detached
  // (Stop() called from inside one of its own tasks) keeps this alive, so it
  // never touches the WorkerPool object after the pool is destroyed.
  struct State {
    std::queue<UniqueTask> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv_task;
    std::condition_variable cv_producer;
    std::condition_variable cv_wait;
    bool stop{false};
    size_t max_queue_size{0};
    size_t active_tasks{0};
    ErrorHandler error_handler;
  };

  std::shared_ptr<State> state_ = std::make_shared<State>();
  std::vector<std::thread> workers_;
  std::queue<UniqueTask> &tasks_ = state_->tasks;
  std::mutex &queue_mutex_ = state_->queue_mutex;
  std::condition_variable &cv_task_ = state_->cv_task;
  std::condition_variable &cv_producer_ = state_->cv_producer;
  std::condition_variable &cv_wait_ = state_->cv_wait;
  bool &stop_ = state_->stop;
  size_t &max_queue_size_ = state_->max_queue_size;
  size_t &active_tasks_ = state_->active_tasks;
  ErrorHandler &error_handler_ = state_->error_handler;

  static void WorkerLoop(std::shared_ptr<State> state) {
    for (;;) {
      UniqueTask task;

      {
        std::unique_lock<std::mutex> lock(state->queue_mutex);

        state->cv_task.wait(lock, [&state] {
          return state->stop || !state->tasks.empty();
        });

        if (state->stop && state->tasks.empty()) {
          return;
        }

        task = std::move(state->tasks.front());
        state->tasks.pop();

        if (state->max_queue_size > 0) {
          state->cv_producer.notify_one();
        }
      }

      // The outer catch block serves as a fallback safety net for the
      // worker thread. While std::packaged_task intercepts and stores
      // task exceptions, any other unexpected exception from the task
      // wrapper itself (e.g. std::bad_function_call) will be caught here
      // to prevent thread termination.
      try {
        task();
      } catch (...) {
        if (state->error_handler) {
          try {
            state->error_handler(std::current_exception());
          } catch (...) {
            // Swallow exception to prevent thread termination
          }
        }
      }
      {
        std::unique_lock<std::mutex> lock(state->queue_mutex);
        if (--state->active_tasks == 0) {
          state->cv_wait.notify_all();
        }
      }
    }
  }

 public:
  /**
   * @brief Constructs a new WorkerPool.
   * @param threads The number of worker threads to spawn. Defaults to hardware
   * concurrency.
   * @param max_queue_size The maximum number of pending tasks allowed in the
   * queue. 0 means unbounded.
   * @param error_handler An optional callback invoked when an unhandled
   * exception escapes a task.
   */
  explicit WorkerPool(size_t threads = std::thread::hardware_concurrency(),
                      size_t max_queue_size = 0,
                      ErrorHandler error_handler = nullptr) {
    max_queue_size_ = max_queue_size;
    error_handler_ = std::move(error_handler);
    if (threads == 0) {
      threads = 1;
    }

    try {
      for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back(&WorkerPool::WorkerLoop, state_);
      }
    } catch (...) {
      {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
      }
      cv_task_.notify_all();
      cv_producer_.notify_all();
      for (std::thread &worker : workers_) {
        if (worker.joinable()) {
          worker.join();
        }
      }
      throw;
    }
  }

  /**
   * @brief Stops the worker pool and terminates worker threads.
   * If called from within a worker thread, that worker thread is detached to
   * avoid self-join deadlock (EDEADLK); it finishes its current task and any
   * queued tasks using shared state, never the destroyed pool object.
   */
  void Stop() {
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (stop_) {
        return;
      }
      stop_ = true;
    }

    cv_task_.notify_all();
    cv_producer_.notify_all();

    const auto current_id = std::this_thread::get_id();
    for (std::thread &worker : workers_) {
      if (worker.joinable()) {
        if (worker.get_id() == current_id) {
          worker.detach();
        } else {
          worker.join();
        }
      }
    }
  }

  /**
   * @brief Destructor that stops the worker pool and joins worker threads.
   */
  ~WorkerPool() {
    Stop();
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
   * @param args The arguments to pass to the function. Arguments are copied
   * by default; wrap in std::ref or std::cref to pass arguments by reference.
   * @return A std::future containing the return value of the function.
   */
  template <class F, class... Args>
  [[nodiscard]] auto Enqueue(F &&f, Args &&...args)
      -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    std::packaged_task<return_type()> task(
        [func = std::forward<F>(f),
         args_tuple =
             std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...),
         state = state_]() mutable {
          try {
            return std::apply(std::move(func), std::move(args_tuple));
          } catch (...) {
            // The inner catch block calls error_handler_ immediately on task
            // exception. Since std::packaged_task captures exceptions and
            // stores them in the future (without propagating them to the worker
            // thread's outer try-catch block), this handler invocation is
            // necessary to ensure background exception logging.
            if (state->error_handler) {
              try {
                state->error_handler(std::current_exception());
              } catch (...) {
                // Swallow error handler exception
              }
            }
            throw;
          }
        });

    std::future<return_type> res = task.get_future();

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (max_queue_size_ > 0) {
        cv_producer_.wait(
            lock, [this] { return stop_ || tasks_.size() < max_queue_size_; });
      }
      if (stop_) {
        throw std::runtime_error("Enqueue called on stopped WorkerPool");
      }
      // Emplace task first; only increment active_tasks_ if emplace succeeds
      // without throwing
      tasks_.emplace(std::move(task));
      ++active_tasks_;
    }

    cv_task_.notify_one();
    return res;
  }

  /**
   * @brief Blocks the calling thread until all enqueued tasks have completed
   * execution.
   * @warning Calling this function from within a task executing on the same
   * pool will deadlock.
   */
  void Wait() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    cv_wait_.wait(lock, [this] { return active_tasks_ == 0; });
  }

  /**
   * @brief Returns the number of worker threads in the pool.
   * @return The pool size.
   */
  size_t Size() const { return workers_.size(); }

  /**
   * @brief Returns the number of pending tasks in the queue.
   * @return The number of pending tasks.
   */
  size_t PendingTasks() const {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return tasks_.size();
  }
};

}  // namespace cppasyncworker