#include "SPSC.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

#include "test_allocator.hpp"

// Note: Some tests may fail with AddressSanitizer due to timing differences
// in how ASAN tracks object destruction in multi-threaded scenarios.
// The regular tests pass without issues.

// Test fixture for wait policy tests
class SPSCAwaitTest : public ::testing::Test {
  protected:
    void SetUp() override {
        allocator = std::make_unique<TestAllocator>();
    }

    void TearDown() override {
        allocator.reset();
    }

    std::unique_ptr<TestAllocator> allocator;
    static constexpr int QUEUE_CAPACITY = 4;
    static constexpr int TEST_TIMEOUT_MS = 1000;
};

// Test PushAwait policy
TEST_F(SPSCAwaitTest, PushAwaitBasicFunctionality) {
    using PushAwaitQueue = SPSC<int, WaitPolicy::PushAwait>;
    PushAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    // Fill the queue to capacity
    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        EXPECT_TRUE(queue.Emplace(i));
    }

    // Next push should fail (queue is full)
    EXPECT_FALSE(queue.Emplace(999));
    EXPECT_EQ(queue.size(), static_cast<size_t>(QUEUE_CAPACITY));

    // Test that Emplace_Await exists and is available for PushAwait policy
    // We can't easily test blocking behavior in a unit test, but we can verify
    // the method compiles and works when space is available

    // Pop one element to make space
    int popped;
    EXPECT_TRUE(queue.Pop(popped));
    EXPECT_EQ(popped, 0);

    // Now Emplace_Await should work without blocking
    queue.Emplace_Await(100);
    EXPECT_EQ(queue.size(), static_cast<size_t>(QUEUE_CAPACITY));

    // Empty the queue before freeing
    int temp;
    while (!queue.empty()) {
        EXPECT_TRUE(queue.Pop(temp));
    }

    queue.Free(*allocator);
}

TEST_F(SPSCAwaitTest, PushAwaitThreadedProducerConsumer) {
    using PushAwaitQueue = SPSC<int, WaitPolicy::PushAwait>;
    PushAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    std::atomic<bool> producer_done{false};
    std::atomic<int> items_produced{0};
    std::atomic<int> items_consumed{0};
    constexpr int TOTAL_ITEMS = 20;

    // Consumer thread - pops items from the queue
    std::thread consumer([&queue, &items_consumed, &producer_done]() {
        int value;
        while (!producer_done || !queue.empty()) {
            if (queue.Pop(value)) {
                items_consumed.fetch_add(1);
                // Small delay to force producer to wait occasionally
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Producer thread - uses Emplace_Await to wait when queue is full
    std::thread producer([&queue, &items_produced, &producer_done]() {
        for (int i = 0; i < TOTAL_ITEMS; ++i) {
            queue.Emplace_Await(i);
            items_produced.fetch_add(1);
        }
        producer_done = true;
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(items_produced.load(), TOTAL_ITEMS);
    EXPECT_EQ(items_consumed.load(), TOTAL_ITEMS);
    EXPECT_TRUE(queue.empty());

    queue.Free(*allocator);
}

// Test PopAwait policy
TEST_F(SPSCAwaitTest, PopAwaitBasicFunctionality) {
    using PopAwaitQueue = SPSC<int, WaitPolicy::PopAwait>;
    PopAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    // Initially empty queue
    EXPECT_TRUE(queue.empty());

    // Normal Pop should return false on empty queue
    int value;
    EXPECT_FALSE(queue.Pop(value));

    // Add some items
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(queue.Emplace(i));
    }

    // Test Pop_Await when items are available (should not block)
    EXPECT_TRUE(queue.Pop_Await(value));
    EXPECT_EQ(value, 0);
    EXPECT_EQ(queue.size(), static_cast<size_t>(2));

    // Empty the rest of the queue
    int temp;
    while (!queue.empty()) {
        EXPECT_TRUE(queue.Pop(temp));
    }

    queue.Free(*allocator);
}

TEST_F(SPSCAwaitTest, PopAwaitThreadedProducerConsumer) {
    using PopAwaitQueue = SPSC<int, WaitPolicy::PopAwait>;
    PopAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    std::atomic<bool> producer_done{false};
    std::atomic<int> items_produced{0};
    std::atomic<int> items_consumed{0};
    constexpr int TOTAL_ITEMS = 20;

    // Producer thread - adds items with delays
    std::thread producer([&queue, &items_produced, &producer_done]() {
        for (int i = 0; i < TOTAL_ITEMS; ++i) {
            EXPECT_TRUE(queue.Emplace(i));
            items_produced.fetch_add(1);
            // Small delay to force consumer to wait occasionally
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
        producer_done = true;
        // End the pop waiting to allow consumer to exit
        queue.End_PopWaiting();
    });

    // Consumer thread - uses Pop_Await to wait when queue is empty
    std::thread consumer([&queue, &items_consumed]() {
        int value;
        while (queue.Pop_Await(value)) {
            items_consumed.fetch_add(1);
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(items_produced.load(), TOTAL_ITEMS);
    EXPECT_EQ(items_consumed.load(), TOTAL_ITEMS);

    queue.Free(*allocator);
}

TEST_F(SPSCAwaitTest, PopAwaitEndWaitingFunctionality) {
    using PopAwaitQueue = SPSC<int, WaitPolicy::PopAwait>;
    PopAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    std::atomic<bool> consumer_exited{false};
    std::atomic<bool> end_waiting_called{false};

    // Consumer thread that waits on empty queue
    std::thread consumer([&queue, &consumer_exited]() {
        int value;
        // This should block initially since queue is empty
        bool result = queue.Pop_Await(value);
        // Should return false when End_PopWaiting is called
        EXPECT_FALSE(result);
        consumer_exited = true;
    });

    // Give consumer time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // End the waiting - this should wake up the consumer
    queue.End_PopWaiting();
    end_waiting_called = true;

    consumer.join();

    EXPECT_TRUE(consumer_exited.load());
    EXPECT_TRUE(end_waiting_called.load());

    queue.Free(*allocator);
}

TEST_F(SPSCAwaitTest, PopAwaitResetWaitingFunctionality) {
    using PopAwaitQueue = SPSC<int, WaitPolicy::PopAwait>;
    PopAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    // End waiting first
    queue.End_PopWaiting();

    // Reset waiting - should allow normal operation again
    queue.Reset_PopWaiting();

    // Add an item and verify Pop_Await works normally
    EXPECT_TRUE(queue.Emplace(42));

    int value;
    EXPECT_TRUE(queue.Pop_Await(value));
    EXPECT_EQ(value, 42);

    queue.Free(*allocator);
}

// Test BothAwait policy (combines PushAwait and PopAwait)
TEST_F(SPSCAwaitTest, BothAwaitBasicFunctionality) {
    using BothAwaitQueue = SPSC<int, WaitPolicy::BothAwait>;
    BothAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    // Test that both Emplace_Await and Pop_Await methods are available

    // Start with empty queue - Pop_Await should be available
    std::thread consumer([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        int value;
        bool result = queue.Pop_Await(value);
        if (result) {
            EXPECT_EQ(value, 100);
        }
    });

    // Add an item using Emplace_Await
    queue.Emplace_Await(100);

    consumer.join();

    queue.End_PopWaiting();  // Clean shutdown
    queue.Free(*allocator);
}

TEST_F(SPSCAwaitTest, BothAwaitComprehensiveTest) {
    using BothAwaitQueue = SPSC<int, WaitPolicy::BothAwait>;
    BothAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    std::atomic<int> items_produced{0};
    std::atomic<int> items_consumed{0};
    constexpr int TOTAL_ITEMS = 15;

    // Producer thread - uses Emplace_Await (will block when queue is full)
    std::thread producer([&queue, &items_produced]() {
        for (int i = 0; i < TOTAL_ITEMS; ++i) {
            queue.Emplace_Await(i);
            items_produced.fetch_add(1);
        }
        // Signal end of production
        queue.End_PopWaiting();
    });

    // Consumer thread - uses Pop_Await (will block when queue is empty)
    std::thread consumer([&queue, &items_consumed]() {
        int value;
        while (queue.Pop_Await(value)) {
            items_consumed.fetch_add(1);
            // Small delay to occasionally fill up the queue
            std::this_thread::sleep_for(std::chrono::microseconds(300));
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(items_produced.load(), TOTAL_ITEMS);
    EXPECT_EQ(items_consumed.load(), TOTAL_ITEMS);

    queue.Free(*allocator);
}

// Test NoWaits policy for comparison (should not have await methods)
TEST_F(SPSCAwaitTest, NoWaitsPolicyLimitations) {
    using NoWaitQueue = SPSC<int, WaitPolicy::NoWaits>;
    NoWaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    // Fill queue
    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        EXPECT_TRUE(queue.Emplace(i));
    }

    // Should fail when full
    EXPECT_FALSE(queue.Emplace(999));

    // Should succeed when empty
    int value;
    EXPECT_TRUE(queue.Pop(value));
    EXPECT_EQ(value, 0);

    // Empty the rest of the queue before freeing
    int temp;
    while (!queue.empty()) {
        EXPECT_TRUE(queue.Pop(temp));
    }

    // Note: Emplace_Await and Pop_Await methods should not be available
    // for NoWaits policy due to requires clauses, but we can't test
    // compilation failures in runtime tests

    queue.Free(*allocator);
}

// Test multiple operations with await
TEST_F(SPSCAwaitTest, MultipleOperationsWithPushAwait) {
    using PushAwaitQueue = SPSC<std::string, WaitPolicy::PushAwait>;
    PushAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    std::vector<std::string> test_data = {"hello", "world", "test", "data", "more", "items"};
    std::vector<std::string> input_span(test_data.begin(), test_data.begin() + 4);

    std::atomic<bool> consumer_ready{false};
    std::vector<std::string> consumed_items;

    std::thread consumer([&queue, &consumed_items, &consumer_ready]() {
        consumer_ready = true;
        std::string item;
        while (consumed_items.size() < 4) {
            if (queue.Pop(item)) {
                consumed_items.push_back(item);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // Wait for consumer to be ready
    while (!consumer_ready) {
        std::this_thread::yield();
    }

    // Use Emplace_Multiple_Await - should handle blocking when queue fills
    std::span<std::string> span(input_span);
    queue.Emplace_Multiple_Await(span);

    consumer.join();

    EXPECT_EQ(consumed_items.size(), static_cast<size_t>(4));
    EXPECT_EQ(consumed_items[0], "hello");
    EXPECT_EQ(consumed_items[1], "world");
    EXPECT_EQ(consumed_items[2], "test");
    EXPECT_EQ(consumed_items[3], "data");

    queue.Free(*allocator);
}

TEST_F(SPSCAwaitTest, MultipleOperationsWithPopAwait) {
    using PopAwaitQueue = SPSC<int, WaitPolicy::PopAwait>;
    PopAwaitQueue queue;
    queue.Allocate(*allocator, QUEUE_CAPACITY);

    std::vector<int> produced_items;
    std::atomic<bool> producer_done{false};

    std::thread producer([&queue, &produced_items, &producer_done]() {
        for (int i = 0; i < 8; ++i) {
            EXPECT_TRUE(queue.Emplace(i));
            produced_items.push_back(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        producer_done = true;
        queue.End_PopWaiting();
    });

    // Use Pop_Multiple_Await - should handle blocking when queue is empty
    std::vector<int> consumed_items;
    consumed_items.reserve(10);

    while (!producer_done || !queue.empty()) {
        queue.Pop_Multiple_Await(consumed_items);
        if (consumed_items.empty()) {
            // If we got nothing and producer is done, we're finished
            if (producer_done)
                break;
        }
    }

    producer.join();

    EXPECT_EQ(consumed_items.size(), produced_items.size());
    for (size_t i = 0; i < consumed_items.size(); ++i) {
        EXPECT_EQ(consumed_items[i], produced_items[i]);
    }

    queue.Free(*allocator);
}

// Performance comparison test (optional)
TEST_F(SPSCAwaitTest, PerformanceComparisonAwaitVsNoWait) {
    constexpr int PERF_ITEMS = 10000;
    constexpr int PERF_CAPACITY = 64;

    // Test NoWait version
    {
        using NoWaitQueue = SPSC<int, WaitPolicy::NoWaits>;
        NoWaitQueue queue;
        TestAllocator no_wait_allocator;
        queue.Allocate(no_wait_allocator, PERF_CAPACITY);

        auto start = std::chrono::high_resolution_clock::now();

        std::thread producer([&queue]() {
            for (int i = 0; i < PERF_ITEMS; ++i) {
                while (!queue.Emplace(i)) {
                    std::this_thread::yield();
                }
            }
        });

        std::thread consumer([&queue]() {
            int value;
            int consumed = 0;
            while (consumed < PERF_ITEMS) {
                if (queue.Pop(value)) {
                    ++consumed;
                }
            }
        });

        producer.join();
        consumer.join();

        auto end = std::chrono::high_resolution_clock::now();
        auto no_wait_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        queue.Free(no_wait_allocator);

        std::cout << "NoWait performance: " << PERF_ITEMS << " items in "
                  << no_wait_duration.count() << " microseconds" << std::endl;
    }

    // Test BothAwait version
    {
        using BothAwaitQueue = SPSC<int, WaitPolicy::BothAwait>;
        BothAwaitQueue queue;
        TestAllocator both_await_allocator;
        queue.Allocate(both_await_allocator, PERF_CAPACITY);

        auto start = std::chrono::high_resolution_clock::now();

        std::thread producer([&queue]() {
            for (int i = 0; i < PERF_ITEMS; ++i) {
                queue.Emplace_Await(i);
            }
            queue.End_PopWaiting();
        });

        std::thread consumer([&queue]() {
            int value;
            int consumed = 0;
            while (consumed < PERF_ITEMS && queue.Pop_Await(value)) {
                ++consumed;
            }
        });

        producer.join();
        consumer.join();

        auto end = std::chrono::high_resolution_clock::now();
        auto both_await_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        queue.Free(both_await_allocator);

        std::cout << "BothAwait performance: " << PERF_ITEMS << " items in "
                  << both_await_duration.count() << " microseconds" << std::endl;
    }
}
