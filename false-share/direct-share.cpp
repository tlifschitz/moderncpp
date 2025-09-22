
#include <atomic>
#include <thread>
#include <iostream>
#include <vector>

int main()
{
    std::atomic<int> var{0};
    constexpr size_t num_threads = 4;
    constexpr size_t max_count = 1 << 27;
    constexpr size_t count_per_thread = max_count / num_threads;

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