#include <gtest/gtest.h>
#include "cppasyncworker.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <vector>

using namespace cppasyncworker;

TEST(WorkerPoolTest, InitializesCorrectly) {
    WorkerPool pool(2);
    // If it doesn't crash or hang, initialization and destruction work
    SUCCEED();
}

TEST(WorkerPoolTest, EnqueuesAndExecutesVoidTasks) {
    WorkerPool pool(2);
    std::atomic<int> counter{0};
    
    for (int i = 0; i < 10; ++i) {
        (void)pool.Enqueue([&counter]() {
            counter++;
        });
    }
    
    // Wait for tasks to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
