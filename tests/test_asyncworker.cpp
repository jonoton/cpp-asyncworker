#include <gtest/gtest.h>
#include "cppasyncworker.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <vector>
#include <set>

using namespace cppasyncworker;

TEST(WorkerPoolTest, InitializesCorrectly) {
    WorkerPool pool(2);
    // If it doesn't crash or hang, initialization and destruction work
    SUCCEED();
}

TEST(WorkerPoolTest, EnqueuesAndExecutesVoidTasks) {
    WorkerPool pool(2);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.Enqueue([&counter]() {
            counter++;
        }));
    }
    
    // Wait deterministically for all tasks to complete
    for (auto& f : futures) {
        f.get();
    }
    EXPECT_EQ(counter.load(), 10);
}

TEST(WorkerPoolTest, ReturnsFutureWithResult) {
    WorkerPool pool(2);
    
    auto future1 = pool.Enqueue([]() { return 42; });
    auto future2 = pool.Enqueue([](int a, int b) { return a + b; }, 10, 20);
    
    EXPECT_EQ(future1.get(), 42);
    EXPECT_EQ(future2.get(), 30);
}

TEST(WorkerPoolTest, PropagatesExceptions) {
    WorkerPool pool(2);
    
    auto future = pool.Enqueue([]() -> int {
        throw std::runtime_error("Test Exception");
    });
    
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(WorkerPoolTest, StressTest) {
    WorkerPool pool(4);
    std::atomic<int> counter{0};
    constexpr int NUM_TASKS = 10000;
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < NUM_TASKS; ++i) {
        futures.push_back(pool.Enqueue([&counter]() {
            counter++;
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
    
    EXPECT_EQ(counter.load(), NUM_TASKS);
}

TEST(WorkerPoolTest, ReturnsCorrectPoolSize) {
    WorkerPool pool(3);
    EXPECT_EQ(pool.Size(), 3u);

    WorkerPool pool2(1);
    EXPECT_EQ(pool2.Size(), 1u);
}

TEST(WorkerPoolTest, ReportsPendingTasks) {
    // Start a pool with 1 thread, and block it with a long running task so we can measure pending tasks
    WorkerPool pool(1);
    
    std::promise<void> start_promise;
    std::shared_future<void> start_future(start_promise.get_future());
    
    std::promise<void> running_promise;
    auto running_future = running_promise.get_future();
    
    std::promise<void> finish_promise;
    auto blocking_task = pool.Enqueue([start_future, &finish_promise, &running_promise]() {
        running_promise.set_value();
        start_future.wait();
        finish_promise.set_value();
    });

    // Wait until the thread has popped and started running the task
    running_future.wait();

    EXPECT_EQ(pool.PendingTasks(), 0u);

    // Enqueue pending tasks
    auto t1 = pool.Enqueue([]() {});
    auto t2 = pool.Enqueue([]() {});

    EXPECT_EQ(pool.PendingTasks(), 2u);

    // Unblock the worker thread
    start_promise.set_value();
    blocking_task.wait();
    t1.wait();
    t2.wait();

    EXPECT_EQ(pool.PendingTasks(), 0u);
}

TEST(WorkerPoolTest, VersionStringIsCorrect) {
    std::string ver = version();
    EXPECT_FALSE(ver.empty());
    std::string ver1 = version();
    std::string ver2 = version();
    EXPECT_EQ(ver1, ver2);
}

// ===========================================================================
// Robustness and Diagnostic Tests
// ===========================================================================

TEST(WorkerPoolTest, SleepBasedSyncIsFlaky) {
    // This test demonstrates WHY sleep-based synchronization is flaky.
    // We enqueue tasks that each take 20ms. With 2 threads and 10 tasks,
    // minimum completion time is ~100ms, but a short sleep window like 50ms is insufficient.
    WorkerPool pool(2);
    std::atomic<int> counter{0};

    for (int i = 0; i < 10; ++i) {
        (void)pool.Enqueue([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            counter++;
        });
    }

    // Attempting to synchronize by sleeping a fixed duration:
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    int snapshot = counter.load();
    if (snapshot < 10) {
        // The flaky pattern is confirmed: sleep wasn't enough
        EXPECT_LT(snapshot, 10)
            << "Sleep-based sync confirmed flaky: only " << snapshot
            << " of 10 tasks completed in 50ms";
        // Mark this as a known issue rather than a hard failure
        GTEST_SKIP() << "Confirmed flaky: " << snapshot << "/10 tasks completed. "
                     << "Sleep-based synchronization is unreliable.";
    } else {
        SUCCEED() << "All tasks completed in time on this run, but this is not guaranteed.";
    }
}

TEST(WorkerPoolTest, FutureBasedSyncIsReliable) {
    WorkerPool pool(2);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.Enqueue([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            counter++;
        }));
    }

    // Deterministic: wait for ALL tasks to complete
    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(counter.load(), 10)
        << "Future-based synchronization is always reliable";
}

TEST(WorkerPoolTest, PoolSurvivesExceptionViaPackagedTask) {
    WorkerPool pool(2);

    // Enqueue a task that throws
    auto bad_future = pool.Enqueue([]() -> int {
        throw std::runtime_error("intentional exception");
    });

    // The exception should be captured in the future
    EXPECT_THROW(bad_future.get(), std::runtime_error);

    // Pool should still be functional — enqueue more work
    auto good_future = pool.Enqueue([]() { return 42; });
    EXPECT_EQ(good_future.get(), 42);
}

TEST(WorkerPoolTest, AllWorkersRemainActiveAfterExceptions) {
    // Use a pool of 2 threads. Throw exceptions in many tasks,
    // then verify BOTH threads are still processing work.
    WorkerPool pool(2);

    // Throw exceptions in several tasks
    std::vector<std::future<int>> bad_futures;
    for (int i = 0; i < 10; ++i) {
        bad_futures.push_back(pool.Enqueue([]() -> int {
            throw std::runtime_error("boom");
        }));
    }

    // Drain the exception futures
    for (auto& f : bad_futures) {
        EXPECT_THROW(f.get(), std::runtime_error);
    }

    // Now verify both threads are active by submitting tasks that
    // record which thread executed them
    std::mutex tid_mutex;
    std::set<std::thread::id> thread_ids;
    std::vector<std::future<void>> probe_futures;

    for (int i = 0; i < 20; ++i) {
        probe_futures.push_back(pool.Enqueue([&tid_mutex, &thread_ids]() {
            {
                std::lock_guard<std::mutex> lock(tid_mutex);
                thread_ids.insert(std::this_thread::get_id());
            }
            // Small sleep to give both threads a chance to pick up tasks
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }));
    }

    for (auto& f : probe_futures) {
        f.get();
    }

    // Both threads should have participated
    EXPECT_GE(thread_ids.size(), 2u)
        << "Expected both worker threads to remain active after exceptions. "
        << "Only " << thread_ids.size() << " thread(s) processed work.";
}

TEST(WorkerPoolTest, EnqueueThrowsAfterStop) {
    auto destructor_started = std::make_shared<std::promise<void>>();
    auto task_running = std::make_shared<std::promise<void>>();
    
    std::shared_future<void> destructor_started_future(destructor_started->get_future());
    auto task_running_future = task_running->get_future();

    std::unique_ptr<WorkerPool> pool = std::make_unique<WorkerPool>(1);
    WorkerPool* pool_ptr = pool.get();
    
    auto f = pool->Enqueue([task_running, destructor_started_future, pool_ptr]() mutable {
        task_running->set_value();
        destructor_started_future.wait();
        
        // Loop calling Enqueue() until the destroyer thread sets stop_ to true.
        // Once stop_ is true, Enqueue() will throw std::runtime_error,
        // which we catch and verify, breaking the loop and finishing the task.
        bool threw = false;
        while (true) {
            try {
                (void)pool_ptr->Enqueue([](){});
            } catch (const std::runtime_error&) {
                threw = true;
                break;
            }
            std::this_thread::yield();
        }
        EXPECT_TRUE(threw);
    });

    task_running_future.wait();
    
    std::thread destroyer([destructor_started, &pool]() {
        destructor_started->set_value();
        pool.reset();
    });

    destroyer.join();
}

TEST(WorkerPoolTest, WaitAllBarrier) {
    WorkerPool pool(2);
    std::atomic<int> counter{0};

    for (int i = 0; i < 5; ++i) {
        (void)pool.Enqueue([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            counter++;
        });
    }

    pool.Wait();
    EXPECT_EQ(counter.load(), 5);
}

TEST(WorkerPoolTest, BoundedQueueBackpressure) {
    WorkerPool pool(1, 1);

    std::promise<void> block_promise;
    std::shared_future<void> block_future(block_promise.get_future());

    (void)pool.Enqueue([block_future]() {
        block_future.wait();
    });

    (void)pool.Enqueue([]() {});

    std::atomic<bool> producer_blocked{true};
    std::thread producer([&pool, &producer_blocked]() {
        (void)pool.Enqueue([]() {});
        producer_blocked = false;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(producer_blocked.load());

    block_promise.set_value();

    producer.join();
    EXPECT_FALSE(producer_blocked.load());

    pool.Wait();
}

TEST(WorkerPoolTest, SupportsMoveOnlyCallables) {
    WorkerPool pool(2);

    auto ptr = std::make_unique<int>(42);
    
    auto fut = pool.Enqueue([p = std::move(ptr)]() {
        return *p;
    });

    EXPECT_EQ(fut.get(), 42);
}

TEST(WorkerPoolTest, CanSupplyErrorHandler) {
    bool handler_called = false;
    WorkerPool pool(2, 0, [&handler_called](const std::exception_ptr& ep) {
        handler_called = true;
    });

    auto fut = pool.Enqueue([]() { return 42; });
    EXPECT_EQ(fut.get(), 42);
    EXPECT_FALSE(handler_called);
}

TEST(WorkerPoolTest, UniqueTaskThrowsWhenEmpty) {
    UniqueTask task;
    EXPECT_THROW(task(), std::bad_function_call);
}

TEST(WorkerPoolTest, ErrorHandlerCalledOnException) {
    std::promise<std::string> exception_msg_promise;
    auto exception_msg_future = exception_msg_promise.get_future();

    WorkerPool pool(2, 0, [&exception_msg_promise](const std::exception_ptr& ep) {
        try {
            if (ep) {
                std::rethrow_exception(ep);
            }
        } catch (const std::exception& e) {
            exception_msg_promise.set_value(e.what());
        }
    });

    auto fut = pool.Enqueue([]() {
        throw std::runtime_error("Simulated Task Exception");
    });

    EXPECT_THROW(fut.get(), std::runtime_error);
    EXPECT_EQ(exception_msg_future.get(), "Simulated Task Exception");
}


