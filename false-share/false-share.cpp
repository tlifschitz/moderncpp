
#include <atomic>
#include <thread>
#include <iostream>
#include <vector>
#include <array>
#include "common.hpp"

int main()
{
    using namespace false_sharing_example;

    std::array<std::atomic<int>, num_threads> vars;
    for (auto& v : vars) {
        v.store(0, std::memory_order_relaxed);
    }

    std::vector<std::thread> threads;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([&vars, i]() {
            for (size_t j = 0; j < count_per_thread; ++j) {
                vars[i].fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    int total = 0;
    for (const auto& v : vars) {
        total += v.load();
    }
    std::cout << "Final value: " << total << std::endl;

    return 0;
}