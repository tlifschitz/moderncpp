# Modern C++ Examples

This repository contains examples and implementations of modern C++ concepts and techniques.

## Projects

### 🚀 Lock-Free Queue (`lockfree-queue/`)
A high-performance Single Producer Single Consumer (SPSC) lock-free queue implementation with different wait policies:
- **NoWaits**: Traditional non-blocking operations
- **PushAwait**: Producers block when queue is full
- **PopAwait**: Consumers block when queue is empty  
- **BothAwait**: Both producers and consumers can block

[📖 Read more](lockfree-queue/README.md)

### 📊 False Sharing Examples (`false-share/`)
Demonstrates the performance impact of false sharing in multi-threaded applications.

[📖 Read more](false-share/README.md)

## Code Formatting

This repository uses **clang-format** to maintain consistent code style across all C++ files.

[📖 Formatting documentation](scripts/README.md)

## Requirements

- **C++20** compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- **CMake** 3.14 or later
- **clang-format** (optional, for code formatting)
- **gtest**  

## Building

Each project can be built independently. See individual project READMEs for specific instructions.


## CI/CD Integration

The project includes GitHub Actions workflows for:
- ✅ **Code formatting checks** - Ensures all code follows the project style
- 🧪 **Automated testing** - Runs all test suites
- 📊 **Performance benchmarks** - Tracks performance over time


---
