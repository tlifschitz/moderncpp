# SPSC Queue - Single Producer Single Consumer Lock-Free Queue

A high-performance, lock-free queue implementation for single producer, single consumer scenarios.

## Features

- **Lock-free**: Uses atomic operations for thread-safe access without mutexes
- **Memory efficient**: Cache-line aligned members to reduce false sharing (push and pop index may be false-shared between producer and consumer thread, impacting in performance)
- **Template-based**: Supports any data type with proper move/copy semantics
- **Batch operations**: Support for bulk insert/remove operations
- **Waiting policies**: Optional blocking operations with different wait strategies
- **Wrap-around indexing**: Efficient circular buffer implementation

## Building and Testing

1. **Build and run tests**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   make test
   ```

## Usage Example

```cpp
#include "SPSC.hpp"
#include <iostream>

// Simple allocator
struct SimpleAllocator {
    std::byte* Allocate(size_t size, size_t alignment) {
        size_t aligned_size = ((size + alignment - 1) / alignment) * alignment;
        return static_cast<std::byte*>(std::aligned_alloc(alignment, aligned_size));
    }
    
    void Free(std::byte* ptr) {
        std::free(ptr);
    }
};

int main() {
    // Create SPSC queue for integers
    SPSC<int> queue;
    SimpleAllocator allocator;
    
    // Allocate memory for 100 elements
    queue.Allocate(allocator, 100);
    
    // Producer operations
    queue.Emplace(42);
    queue.Emplace(13, 37);  // Multiple arguments
    
    // Consumer operations
    int value;
    if (queue.Pop(value)) {
        std::cout << "Popped: " << value << std::endl;
    }
    
    // Batch operations
    std::vector<int> data = {1, 2, 3, 4, 5};
    std::span<int> span(data);
    auto remaining = queue.Emplace_Multiple(span);
    
    std::vector<int> output;
    queue.Pop_Multiple(output);
    
    // Cleanup
    queue.Free(allocator);
    return 0;
}
```

## Test Coverage

The unit tests cover:

- ✅ Basic functionality (allocation, emplace, pop)
- ✅ Edge cases (empty/full queue, invalid operations)
- ✅ Complex data types (strings, custom objects)
- ✅ Batch operations (multiple insert/remove)
- ✅ Concurrency (producer-consumer scenarios)
- ✅ Memory management (proper cleanup)
- ✅ Performance benchmarking
- ✅ Stress testing with random operations

Run tests with: `make test` or `ctest` (if using CMake)
