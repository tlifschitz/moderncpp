#pragma once

#include <cstddef>

// Common configuration for false sharing benchmarks
namespace false_sharing_example {
// Number of threads to use for multithreaded examples
constexpr size_t num_threads = 8;

// Total number of increments across all threads
constexpr size_t max_count = 1 << 27;  // 134,217,728

// Number of increments per thread
constexpr size_t count_per_thread = max_count / num_threads;

// Cache line size for padding
#ifdef __cpp_lib_hardware_interference_size >= 201603
constexpr size_t cache_line_size = std::hardware_destructive_interference_size;
#else
constexpr size_t cache_line_size = 64;  // Common cache line size
#endif
}  // namespace false_sharing_example
