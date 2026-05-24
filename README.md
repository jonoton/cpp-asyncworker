# cpp-asyncworker

`cpp-asyncworker` is a lightweight, thread-safe, and header-only C++ library for managing asynchronous tasks using a worker pool.

## Features

- **Header-Only & Easy Integration:** Simply copy `cppasyncworker.hpp` into your project, or integrate it seamlessly via modern CMake (`FetchContent` or `add_subdirectory()`).
- **Flexible Task Execution:** Enqueue any callable object—lambdas, function pointers, functors, or `std::bind` expressions—along with any number of arguments.
- **Futures Integration:** Returns a `std::future` for every enqueued task, allowing you to easily retrieve return values or block until completion.
- **Exception Propagation:** Exceptions thrown within worker threads are safely caught and re-thrown in the calling thread when you call `.get()` on the task's future.
- **Hardware-Aware Concurrency:** Automatically defaults to the optimal number of worker threads based on your system's hardware concurrency.
- **Standard C++:** Built purely on standard `<thread>`, `<mutex>`, `<condition_variable>`, and `<future>` to ensure maximum compatibility and thread safety without any external dependencies.

## Quick Start

```cpp
#include <iostream>
#include "cppasyncworker.hpp"

int main() {
    // Create a worker pool that defaults to hardware concurrency threads
    cppasyncworker::WorkerPool pool;

    // Enqueue a task
    auto future = pool.Enqueue([](int a, int b) {
        return a + b;
    }, 10, 20);

    // Get the result
    std::cout << "Result: " << future.get() << std::endl;

    return 0;
}
```

## CMake Integration

You can easily integrate `cpp-asyncworker` into your CMake project using `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
    cppasyncworker
    GIT_REPOSITORY https://github.com/jonoton/cpp-asyncworker.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(cppasyncworker)

target_link_libraries(your_target PRIVATE cppasyncworker::cppasyncworker)
```

## Tests and Examples

The library includes GoogleTest-based unit tests and usage examples. To build and run them:

```bash
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

## Documentation

Full documentation and examples are available at [https://jonoton.github.io/cpp-asyncworker/](https://jonoton.github.io/cpp-asyncworker/).
