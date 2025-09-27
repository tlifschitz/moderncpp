// POLICIES
// E.g. MPSC: Multiple Producers, Single Consumer
enum class ThreadsPolicy { SPSC = 0, SPMC, MPSC, MPMC };

enum class WaitPolicy { NoWaits = 0, PushAwait, PopAwait, BothAwait };

constexpr bool Await_Pushes(WaitPolicy aWaiting) {
    return (aWaiting == WaitPolicy::PushAwait) || (aWaiting == WaitPolicy::BothAwait);
}

constexpr bool Await_Pops(WaitPolicy aWaiting) {
    return (aWaiting == WaitPolicy::PopAwait) || (aWaiting == WaitPolicy::BothAwait);
}

// CLASS DECLARATION
template <typename DataType, ThreadsPolicy Threading, WaitPolicy Waiting = WaitPolicy::NoWaits>
class Queue;

#pragma once
#include <cstdlib>
#include <iostream>
#include <memory>
#include <span>
#include <sstream>

// Simple Assert function
inline void Assert(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::abort();
    }
}

template <typename T>
inline void Assert(bool condition, const std::string& message, const T& value) {
    if (!condition) {
        std::cerr << message;
        if (message.find("{}") != std::string::npos) {
            std::cerr << " (value: " << value << ")";
        }
        std::cerr << std::endl;
        std::abort();
    }
}

// Span type alias for compatibility
template <typename T>
using Span = std::span<T>;

// Fallback for hardware_destructive_interference_size if not available
#ifdef __cpp_lib_hardware_interference_size >= 201603
constexpr std::size_t hardware_destructive_interference_size =
    std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;  // Common cache line size
#endif
