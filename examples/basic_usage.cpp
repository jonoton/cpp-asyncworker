#include <iostream>
#include <chrono>
#include <thread>
#include "cppasyncworker.hpp"

int main() {
    std::cout << "Starting basic usage example..." << std::endl;

    // Create a worker pool that defaults to hardware concurrency threads
    cppasyncworker::WorkerPool pool;

    // Enqueue a few void tasks
    for (int i = 0; i < 5; ++i) {
        (void)pool.Enqueue([i]() {
            std::cout << "Task " << i << " executing on thread " << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Task " << i << " finished" << std::endl;
        });
    }

    std::cout << "Tasks enqueued, waiting for all tasks to complete via pool.Wait()..." << std::endl;
    pool.Wait();
    std::cout << "All tasks finished! Exiting." << std::endl;

    return 0;
}
