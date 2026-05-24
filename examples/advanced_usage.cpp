#include <iostream>
#include <vector>
#include <future>
#include <stdexcept>
#include "cppasyncworker.hpp"

int main() {
    std::cout << "Starting advanced usage example..." << std::endl;

    cppasyncworker::WorkerPool pool(4);
    std::vector<std::future<int>> results;

    // Enqueue tasks that return a value
    for (int i = 0; i < 5; ++i) {
        results.push_back(pool.Enqueue([i]() {
            if (i == 3) {
                throw std::runtime_error("Simulated error in task 3");
            }
            return i * 10;
        }));
    }

    // Retrieve results and handle exceptions
    for (size_t i = 0; i < results.size(); ++i) {
        try {
            int value = results[i].get();
            std::cout << "Result " << i << ": " << value << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception in task " << i << ": " << e.what() << std::endl;
        }
    }

    return 0;
}
