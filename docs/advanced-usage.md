---
layout: default
---

# Advanced Usage

The `Enqueue` method returns a `std::future`, which allows you to retrieve return values from your asynchronous tasks or handle exceptions thrown within the workers.

## Retrieving Results

You can wait for a task to complete and retrieve its result using the returned `std::future`.

```cpp
#include <iostream>
#include "cppasyncworker.hpp"

int main() {
    cppasyncworker::WorkerPool pool;

    // Enqueue a task that returns a value
    auto future_result = pool.Enqueue([](int a, int b) {
        return a + b;
    }, 10, 20);

    // Wait for the result and print it
    std::cout << "Result: " << future_result.get() << std::endl;

    return 0;
}
```

## Handling Exceptions

If a task throws an exception, it is caught by the internal `std::packaged_task` and rethrown when you call `.get()` on the corresponding `std::future`.

```cpp
#include <iostream>
#include <stdexcept>
#include "cppasyncworker.hpp"

int main() {
    cppasyncworker::WorkerPool pool;

    auto future_result = pool.Enqueue([]() {
        throw std::runtime_error("Something went wrong in the background!");
        return 42;
    });

    try {
        // This will rethrow the exception
        future_result.get();
    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }

    return 0;
}
```

## Support for Move-Only Callables

The `WorkerPool` uses a custom type-erased `UniqueTask` wrapper, allowing you to enqueue move-only callables (such as lambdas capturing a `std::unique_ptr`). This enables efficient resource transfer to background threads without unnecessary copying.

```cpp
#include <iostream>
#include <memory>
#include "cppasyncworker.hpp"

int main() {
    cppasyncworker::WorkerPool pool;

    // Create a unique pointer (move-only)
    auto resource = std::make_unique<int>(100);

    // Enqueue a lambda capturing 'resource' by move
    auto future = pool.Enqueue([r = std::move(resource)]() {
        return *r + 50;
    });

    std::cout << "Resource value processed: " << future.get() << std::endl; // Prints 150

    return 0;
}
```

## Bounded Queues and Backpressure

To prevent unbounded memory consumption under high load (e.g., a fast producer enqueuing tasks much quicker than worker threads can process them), you can configure a maximum capacity for the task queue in the constructor.

When `max_queue_size` is non-zero and the task queue is full, any subsequent call to `Enqueue` will block the producer thread until a worker thread executes a task and makes room in the queue.

```cpp
#include <iostream>
#include "cppasyncworker.hpp"

int main() {
    // 2 worker threads, maximum queue size of 5 tasks
    cppasyncworker::WorkerPool pool(2, 5);

    // The producer can enqueue up to 7 tasks safely (2 executing, 5 waiting in queue)
    for (int i = 0; i < 7; ++i) {
        pool.Enqueue([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
    }

    // This 8th Enqueue will block the main thread until at least one task finishes
    std::cout << "Enqueuing 8th task (might block due to backpressure)..." << std::endl;
    pool.Enqueue([]() {
        std::cout << "8th task executed!" << std::endl;
    });

    pool.Wait();
    return 0;
}
```

## Custom Unhandled Exception Callback

While exceptions are normally propagated through the `std::future` returned by `Enqueue`, you might discard or ignore the future for fire-and-forget tasks. To ensure that unhandled exceptions from these tasks do not escape silently, you can register a custom `ErrorHandler` callback.

The error handler will be executed in the context of the worker thread that caught the exception.

```cpp
#include <iostream>
#include <stdexcept>
#include "cppasyncworker.hpp"

int main() {
    // Define an error callback
    auto on_error = [](const std::exception_ptr& ex_ptr) {
        try {
            if (ex_ptr) {
                std::rethrow_exception(ex_ptr);
            }
        } catch (const std::exception& e) {
            std::cerr << "[Error Callback] Caught unhandled exception: " << e.what() << std::endl;
        }
    };

    // Instantiate pool with 4 threads, unbounded queue (0), and our error handler
    cppasyncworker::WorkerPool pool(4, 0, on_error);

    // Enqueue a fire-and-forget task that throws (we discard the returned future)
    (void)pool.Enqueue([]() {
        throw std::runtime_error("Fatal background task exception!");
    });

    pool.Wait();
    return 0;
}
```

