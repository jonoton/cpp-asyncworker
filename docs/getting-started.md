---
layout: default
---

# Getting Started

## Prerequisites

- A C++17 (or later) compliant compiler (e.g., GCC, Clang, MSVC).
- Standard C++ Library with threading support (`<thread>`, `<future>`, `<mutex>`, `<condition_variable>`).

## Installation

`cpp-asyncworker` is a header-only library. To use it, simply copy the `cppasyncworker.hpp` file into your project's include directory.

```bash
cp cppasyncworker.hpp /path/to/your/project/include/
```

Then, include it in your C++ files:

```cpp
#include "cppasyncworker.hpp"
```

There are no external dependencies or build steps required.

### CMake Integration

If you are using CMake, you can easily integrate `cpp-asyncworker` into your project using `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
    cppasyncworker
    GIT_REPOSITORY https://github.com/jonoton/cpp-asyncworker.git
    GIT_TAG        main # Or a specific tag
)
FetchContent_MakeAvailable(cppasyncworker)

# Link to your target
target_link_libraries(your_target PRIVATE cppasyncworker::cppasyncworker)
```

Alternatively, you can clone the repository and use `add_subdirectory()`:

```cmake
add_subdirectory(path/to/cpp-asyncworker)
target_link_libraries(your_target PRIVATE cppasyncworker::cppasyncworker)
```
