
#include <array>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

#include "common.hpp"

struct alignas(false_sharing_example::cache_line_size) PaddedAtomicInt {
    std::atomic<int> value{0};
};

int main() {
    using namespace false_sharing_example;

    std::array<PaddedAtomicInt, num_threads> vars;
    std::vector<std::thread>                 threads;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([&vars, i]() {
            for (size_t j = 0; j < count_per_thread; ++j) {
                vars[i].value.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    int total = 0;
    for (const auto& v : vars) {
        total += v.value.load();
    }
    std::cout << "Final value: " << total << std::endl;

    return 0;
}