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
