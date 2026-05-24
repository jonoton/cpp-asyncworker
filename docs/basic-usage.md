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
