
#include "common.hpp"
#include <atomic>
#include <iostream>
#include <thread>

// How to compile this code with g++?
// g++ -std=c++20 -pthread -o secuencial secuencial.cpp

int main() {
    using namespace false_sharing_example;

    std::atomic<int> var{0};
    std::thread t1([&var]() {
        for (size_t i = 0; i < max_count; ++i) {
            var.fetch_add(1, std::memory_order_relaxed);
        }
    });

    t1.join();

    std::cout << "Final value: " << var.load() << std::endl;

    return 0;
}