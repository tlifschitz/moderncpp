
#include <atomic>
#include <thread>
#include <iostream>
#include <vector>
#include <array>

#ifdef __cpp_lib_hardware_interference_size
    constexpr size_t cache_line_size = std::hardware_destructive_interference_size;
#else
    constexpr size_t cache_line_size = 64; // Common cache line size
#endif

struct alignas(cache_line_size) PaddedAtomicInt {
    std::atomic<int> value{0};
};

int main()
{

    constexpr size_t num_threads = 4;
    constexpr size_t max_count = 1 << 27;
    constexpr size_t count_per_thread = max_count / num_threads;

    std::array<PaddedAtomicInt, num_threads> vars;
    std::vector<std::thread> threads;
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