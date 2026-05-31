---
layout: default
---

# cpp-asyncworker

Welcome to the **cpp-asyncworker** documentation! This is a lightweight, thread-safe, and header-only C++ library for managing asynchronous tasks using a worker pool.

## Features

- **Header-Only & Easy Integration:** Simply copy `cppasyncworker.hpp` into your project, or integrate it seamlessly via modern CMake (`FetchContent` or `add_subdirectory()`).
- **Move-Only Callable Support:** Supports enqueuing move-only callables (like lambdas capturing `std::unique_ptr`) using custom type-erased `UniqueTask` wrappers.
- **Flexible Task Execution:** Enqueue any callable object—lambdas, function pointers, functors, or `std::bind` expressions—along with any number of arguments.
- **Futures Integration:** Returns a `std::future` for every enqueued task, allowing you to easily retrieve return values or block until completion.
- **Global Synchronization:** Blocks the calling thread with the `Wait()` method until all enqueued tasks have finished execution.
- **Bounded Queue & Backpressure:** Set a maximum queue size to block calling/producer threads when the queue is full, preventing unbounded memory growth.
- **Custom Error Handling:** Supply an optional unhandled exception callback (`ErrorHandler`) to capture exceptions thrown in the background threads.
- **Exception Propagation:** Exceptions thrown within worker threads are safely caught and re-thrown in the calling thread when you call `.get()` on the task's future.
- **Hardware-Aware Concurrency:** Automatically defaults to the optimal number of worker threads based on your system's hardware concurrency.
- **Standard C++:** Built purely on standard C++17 library threads and synchronization primitives without any external dependencies.
- **Utility Queries:** Easily check the number of worker threads (`Size()`) and the current count of pending tasks in the queue (`PendingTasks()`).

## Table of Contents

- [Getting Started]({{ '/getting-started.html' | relative_url }}) - Installation and requirements.
- [Basic Usage]({{ '/basic-usage.html' | relative_url }}) - Basic usage of the `WorkerPool`.
- [Advanced Usage]({{ '/advanced-usage.html' | relative_url }}) - Handling return values, futures, and synchronization.
