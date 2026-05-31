---
layout: default
---

# Basic Usage

Using `cpp-asyncworker` is straightforward. You instantiate a `WorkerPool` and enqueue tasks.

## Creating a Worker Pool

You can create a `WorkerPool` with a specific number of threads, or use the default constructor to automatically detect and use the hardware concurrency level.

```cpp
#include <iostream>
#include "cppasyncworker.hpp"

int main() {
    // Create a worker pool with 4 threads
    cppasyncworker::WorkerPool pool(4);

    // Or use default hardware concurrency
    // cppasyncworker::WorkerPool pool;

    return 0;
}
```

## Enqueueing Tasks

You can enqueue lambda expressions, functions, or any callable object into the pool.

```cpp
#include <iostream>
#include "cppasyncworker.hpp"

int main() {
    cppasyncworker::WorkerPool pool(4);

    // Enqueue a simple task
    pool.Enqueue([]() {
        std::cout << "Task executed!" << std::endl;
    });

    // Enqueue a task with arguments
    int value = 42;
    pool.Enqueue([](int v) {
        std::cout << "Task executed with value: " << v << std::endl;
    }, value);

    // The WorkerPool destructor will wait for all threads to finish their current tasks
    return 0;
}
```

## Global Synchronization with `Wait()`

If you want to wait for all currently enqueued tasks to complete without destroying the worker pool, you can use the `Wait()` method. This blocks the calling thread until the worker queue is empty and all threads have finished executing active tasks.

```cpp
#include <iostream>
#include <chrono>
#include <thread>
#include "cppasyncworker.hpp"

int main() {
    cppasyncworker::WorkerPool pool(2);

    for (int i = 0; i < 5; ++i) {
        pool.Enqueue([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "Task " << i << " finished" << std::endl;
        });
    }

    std::cout << "Waiting for all tasks to finish..." << std::endl;
    pool.Wait(); // Blocks until all 5 tasks are fully complete
    std::cout << "All tasks finished!" << std::endl;

    return 0;
}
```

## Querying Worker Pool State

You can query the status and size of the `WorkerPool` using the following methods:

- `Size()`: Returns the total number of worker threads active in the pool.
- `PendingTasks()`: Returns the number of tasks currently waiting in the queue to be picked up by worker threads.

```cpp
cppasyncworker::WorkerPool pool(3);

std::cout << "Number of worker threads: " << pool.Size() << std::endl; // Prints 3

// Enqueue a task
pool.Enqueue([]() { std::this_thread::sleep_for(std::chrono::seconds(1)); });

std::cout << "Pending tasks in queue: " << pool.PendingTasks() << std::endl;
```
