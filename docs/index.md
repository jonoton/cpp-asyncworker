---
layout: default
---

# cpp-asyncworker

Welcome to the **cpp-asyncworker** documentation! This is a lightweight, thread-safe, and header-only C++ library for managing asynchronous tasks using a worker pool.

## Features

- **Header-Only & Easy Integration:** Simply copy `cppasyncworker.hpp` into your project, or integrate it seamlessly via modern CMake (`FetchContent` or `add_subdirectory()`).
- **Flexible Task Execution:** Enqueue any callable object—lambdas, function pointers, functors, or `std::bind` expressions—along with any number of arguments.
- **Futures Integration:** Returns a `std::future` for every enqueued task, allowing you to easily retrieve return values or block until completion.
- **Exception Propagation:** Exceptions thrown within worker threads are safely caught and re-thrown in the calling thread when you call `.get()` on the task's future.
- **Hardware-Aware Concurrency:** Automatically defaults to the optimal number of worker threads based on your system's hardware concurrency.
- **Standard C++:** Built purely on standard `<thread>`, `<mutex>`, `<condition_variable>`, and `<future>` to ensure maximum compatibility and thread safety without any external dependencies.

## Table of Contents

- [Getting Started]({{ '/getting-started.html' | relative_url }}) - Installation and requirements.
- [Basic Usage]({{ '/basic-usage.html' | relative_url }}) - Basic usage of the `WorkerPool`.
- [Advanced Usage]({{ '/advanced-usage.html' | relative_url }}) - Handling return values, futures, and synchronization.
