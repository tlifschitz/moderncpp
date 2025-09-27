
#include "common.hpp"
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    using namespace false_sharing_example;

    std::atomic<int> var{0};

    std::vector<std::thread> threads;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([&var]() {
            for (size_t i = 0; i < count_per_thread; ++i) {
                var.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Final value: " << var.load() << std::endl;

    return 0;
}