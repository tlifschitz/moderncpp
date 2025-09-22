# False Sharing Performance Examples

This directory contains C++ examples demonstrating the performance impact of **false sharing** in multithreaded applications.

## What is False Sharing?

False sharing occurs when multiple threads access different variables that happen to reside on the same cache line. Even though the threads are accessing different data, the CPU cache coherency protocol treats the entire cache line as a single unit, causing unnecessary cache invalidations and performance degradation.

## Examples

### 1. `secuencial.cpp` - Sequential Baseline
Single-threaded implementation using one atomic variable. This serves as our baseline for comparison.

### 2. `direct-share.cpp` - True Sharing
Multiple threads all increment the same atomic variable. This demonstrates **true sharing** where threads genuinely compete for the same memory location.

### 3. `false-share.cpp` - False Sharing Problem
Each thread has its own atomic variable in an array, but they're packed together in memory. This creates false sharing because the atomic integers likely share cache lines.

```cpp
std::array<std::atomic<int>, num_threads> vars;  // Packed together!
```

### 4. `no-share.cpp` - False Sharing Solution
Each thread has its own atomic variable, but they're padded to occupy separate cache lines using `alignas(cache_line_size)`.

```cpp
struct alignas(cache_line_size) PaddedAtomicInt {
    std::atomic<int> value{0};
};
```

## Expected Performance Results

When you run the benchmark, you should see:

1. **Sequential**: Baseline performance (single thread)
2. **Direct Share**: Slower due to true contention
3. **False Share**: **Slowest** - false sharing penalty
4. **No Share**: **Fastest** - optimal parallel performance

## Building and Running

Use the provided test script to build and benchmark all examples:

```bash
chmod +x test.sh
./test.sh
```

Or build manually:

```bash
g++ -std=c++20 -pthread -O2 -o secuencial secuencial.cpp
g++ -std=c++20 -pthread -O2 -o direct-share direct-share.cpp
g++ -std=c++20 -pthread -O2 -o false-share false-share.cpp
g++ -std=c++20 -pthread -O2 -o no-share no-share.cpp
```

### MacOS results

```
Benchmark 1: ./build/secuencial
  Time (mean ± σ):     295.8 ms ±   1.3 ms    [User: 294.0 ms, System: 1.1 ms]
  Range (min … max):   294.3 ms … 297.5 ms    5 runs
 
Benchmark 2: ./build/direct-share
  Time (mean ± σ):     667.2 ms ±  24.1 ms    [User: 1319.5 ms, System: 15.7 ms]
  Range (min … max):   639.2 ms … 696.3 ms    5 runs
 
Benchmark 3: ./build/false-share
  Time (mean ± σ):      1.084 s ±  0.312 s    [User: 3.181 s, System: 0.007 s]
  Range (min … max):    0.696 s …  1.388 s    5 runs
 
Benchmark 4: ./build/no-share
  Time (mean ± σ):     108.3 ms ±   0.8 ms    [User: 314.3 ms, System: 1.5 ms]
  Range (min … max):   107.8 ms … 109.6 ms    5 runs
 
Summary
  ./build/no-share ran
    2.73 ± 0.02 times faster than ./build/secuencial
    6.16 ± 0.23 times faster than ./build/direct-share
   10.01 ± 2.88 times faster than ./build/false-share
Build complete.
```

## Ubuntu results
```
Benchmarking secuencial...
Final value: 134217728
  Elapsed time:    260.0 ms
  CPU time:        260.0 ms (user: 260.0 ms, system: 0.0 ms)
  CPU usage:         99%%
  Memory:           3968 KB

Benchmarking direct-share...
Final value: 134217728
  Elapsed time:   1880.0 ms
  CPU time:       7470.0 ms (user: 7470.0 ms, system: 0.0 ms)
  CPU usage:        395%%
  Memory:           3996 KB

Benchmarking false-share...
Final value: 134217728
  Elapsed time:   1920.0 ms
  CPU time:       7680.0 ms (user: 7680.0 ms, system: 0.0 ms)
  CPU usage:        399%%
  Memory:           3860 KB

Benchmarking no-share...
Final value: 134217728
  Elapsed time:     80.0 ms
  CPU time:        310.0 ms (user: 310.0 ms, system: 0.0 ms)
  CPU usage:        372%%
  Memory:           3992 KB
```

## Key Takeaways

- **Cache lines** (typically 64 bytes) are the unit of cache coherency
- **False sharing** can make parallel code slower than sequential code
- **Padding/alignment** can eliminate false sharing at the cost of memory usage
- Always profile multithreaded code - performance can be counterintuitive!

## System Requirements

- C++20 compatible compiler (for `std::hardware_destructive_interference_size`)
- POSIX threads support
- For benchmarking: `hyperfine` (macOS) or `time` (Linux)

---

*This example demonstrates why understanding memory layout is crucial for high-performance parallel programming.*
