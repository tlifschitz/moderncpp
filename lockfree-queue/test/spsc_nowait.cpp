#include "SPSC.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include "test_allocator.hpp"

// Test fixture for SPSC queue tests
class SPSCQueueTest : public ::testing::Test {
  protected:
    void SetUp() override {
        allocator_ = std::make_unique<TestAllocator>();
    }

    void TearDown() override {
        if (queue_ && queue_->Is_Allocated()) {
            queue_->Free(*allocator_);
        }
    }

    template <typename DataType, WaitPolicy Waiting = WaitPolicy::NoWaits>
    void CreateQueue(int capacity) {
        queue_ = std::make_unique<Queue<DataType, ThreadsPolicy::SPSC, Waiting>>();
        queue_->Allocate(*allocator_, capacity);
    }

    std::unique_ptr<TestAllocator> allocator_;
    std::unique_ptr<Queue<int, ThreadsPolicy::SPSC, WaitPolicy::NoWaits>> queue_;
};

// Basic functionality tests
TEST_F(SPSCQueueTest, ConstructorAndDestructor) {
    Queue<int, ThreadsPolicy::SPSC, WaitPolicy::NoWaits> queue;
    EXPECT_FALSE(queue.Is_Allocated());
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), static_cast<size_t>(0));
}

TEST_F(SPSCQueueTest, AllocationAndDeallocation) {
    Queue<int, ThreadsPolicy::SPSC, WaitPolicy::NoWaits> queue;

    // Test allocation
    queue.Allocate(*allocator_, 10);
    EXPECT_TRUE(queue.Is_Allocated());
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), static_cast<size_t>(0));
    EXPECT_EQ(allocator_->allocated_count(), static_cast<size_t>(1));

    // Test deallocation
    queue.Free(*allocator_);
    EXPECT_FALSE(queue.Is_Allocated());
    EXPECT_EQ(allocator_->allocated_count(), static_cast<size_t>(0));
}

TEST_F(SPSCQueueTest, InvalidAllocation) {
    Queue<int, ThreadsPolicy::SPSC, WaitPolicy::NoWaits> queue;

    // Test allocation with zero capacity
    EXPECT_DEATH(queue.Allocate(*allocator_, 0), "Invalid capacity");

    // Test allocation with negative capacity
    EXPECT_DEATH(queue.Allocate(*allocator_, -1), "Invalid capacity");
}

TEST_F(SPSCQueueTest, DoubleAllocation) {
    CreateQueue<int>(10);

    // Attempting to allocate again should fail
    EXPECT_DEATH(queue_->Allocate(*allocator_, 5), "Can't allocate while still owning memory");
}

TEST_F(SPSCQueueTest, FreeWithoutAllocation) {
    Queue<int, ThreadsPolicy::SPSC, WaitPolicy::NoWaits> queue;

    // Attempting to free without allocation should fail
    EXPECT_DEATH(queue.Free(*allocator_), "No memory to free");
}

// Single element operations
TEST_F(SPSCQueueTest, SingleElementOperations) {
    CreateQueue<int>(10);

    // Test emplace
    EXPECT_TRUE(queue_->Emplace(42));
    EXPECT_FALSE(queue_->empty());
    EXPECT_EQ(queue_->size(), static_cast<size_t>(1));

    // Test pop
    int value;
    EXPECT_TRUE(queue_->Pop(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(queue_->empty());
    EXPECT_EQ(queue_->size(), static_cast<size_t>(0));

    // Test pop from empty queue
    EXPECT_FALSE(queue_->Pop(value));
}

TEST_F(SPSCQueueTest, MultipleElements) {
    CreateQueue<int>(5);

    // Fill the queue
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(queue_->Emplace(i));
        EXPECT_EQ(queue_->size(), static_cast<size_t>(i + 1));
    }

    // Queue should be full now
    EXPECT_FALSE(queue_->Emplace(100));
    EXPECT_EQ(queue_->size(), static_cast<size_t>(5));

    // Pop all elements
    for (int i = 0; i < 5; ++i) {
        int value;
        EXPECT_TRUE(queue_->Pop(value));
        EXPECT_EQ(value, i);
        EXPECT_EQ(queue_->size(), static_cast<size_t>(4 - i));
    }

    EXPECT_TRUE(queue_->empty());
}

TEST_F(SPSCQueueTest, WrapAround) {
    CreateQueue<int>(3);

    // Fill and empty multiple times to test wrap-around
    for (int cycle = 0; cycle < 10; ++cycle) {
        // Fill
        for (int i = 0; i < 3; ++i) {
            EXPECT_TRUE(queue_->Emplace(cycle * 3 + i));
        }

        // Empty
        for (int i = 0; i < 3; ++i) {
            int value;
            EXPECT_TRUE(queue_->Pop(value));
            EXPECT_EQ(value, cycle * 3 + i);
        }

        EXPECT_TRUE(queue_->empty());
    }
}

// Complex data types
TEST_F(SPSCQueueTest, StringOperations) {
    Queue<std::string, ThreadsPolicy::SPSC, WaitPolicy::NoWaits> string_queue;
    string_queue.Allocate(*allocator_, 5);

    // Test with strings
    std::vector<std::string> test_strings = {"hello", "world", "cpp", "queue", "test"};

    // Emplace strings
    for (const auto& str : test_strings) {
        EXPECT_TRUE(string_queue.Emplace(str));
    }

    // Pop and verify
    for (size_t i = 0; i < test_strings.size(); ++i) {
        std::string value;
        EXPECT_TRUE(string_queue.Pop(value));
        EXPECT_EQ(value, test_strings[i]);
    }

    string_queue.Free(*allocator_);
}

TEST_F(SPSCQueueTest, CustomObjectOperations) {
    struct TestObject {
        int id;
        std::string name;

        TestObject(int i, std::string n) : id(i), name(std::move(n)) {}

        bool operator==(const TestObject& other) const {
            return id == other.id && name == other.name;
        }
    };

    Queue<TestObject, ThreadsPolicy::SPSC, WaitPolicy::NoWaits> obj_queue;
    obj_queue.Allocate(*allocator_, 3);

    // Test emplacement with constructor arguments
    EXPECT_TRUE(obj_queue.Emplace(1, "first"));
    EXPECT_TRUE(obj_queue.Emplace(2, "second"));
    EXPECT_TRUE(obj_queue.Emplace(3, "third"));

    // Verify objects
    TestObject obj(0, "");
    EXPECT_TRUE(obj_queue.Pop(obj));
    EXPECT_EQ(obj.id, 1);
    EXPECT_EQ(obj.name, "first");

    EXPECT_TRUE(obj_queue.Pop(obj));
    EXPECT_EQ(obj.id, 2);
    EXPECT_EQ(obj.name, "second");

    EXPECT_TRUE(obj_queue.Pop(obj));
    EXPECT_EQ(obj.id, 3);
    EXPECT_EQ(obj.name, "third");

    obj_queue.Free(*allocator_);
}

// Batch operations tests
TEST_F(SPSCQueueTest, EmplaceMultiple) {
    CreateQueue<int>(10);

    std::vector<int> input_data = {1, 2, 3, 4, 5};
    std::span<int> input_span(input_data);

    // Test batch emplace
    auto remaining = queue_->Emplace_Multiple(input_span);
    EXPECT_TRUE(remaining.empty());  // All should be emplaced
    EXPECT_EQ(queue_->size(), static_cast<size_t>(5));

    // Verify all elements
    for (int expected : input_data) {
        int value;
        EXPECT_TRUE(queue_->Pop(value));
        EXPECT_EQ(value, expected);
    }
}

TEST_F(SPSCQueueTest, EmplaceMultiplePartial) {
    CreateQueue<int>(3);  // Smaller capacity

    std::vector<int> input_data = {1, 2, 3, 4, 5};  // More data than capacity
    std::span<int> input_span(input_data);

    // Test partial batch emplace
    auto remaining = queue_->Emplace_Multiple(input_span);
    EXPECT_EQ(remaining.size(), static_cast<size_t>(2));  // 2 elements should remain
    EXPECT_EQ(queue_->size(), static_cast<size_t>(3));    // Queue should be full

    // Check remaining elements
    EXPECT_EQ(remaining[0], 4);
    EXPECT_EQ(remaining[1], 5);

    // Clean up the queue before destruction
    int value;
    while (queue_->Pop(value)) {
        // Empty the queue
    }
}

TEST_F(SPSCQueueTest, PopMultiple) {
    CreateQueue<int>(10);

    // Fill queue with test data
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(queue_->Emplace(i));
    }

    // Test batch pop
    std::vector<int> output_data;
    output_data.reserve(10);
    queue_->Pop_Multiple(output_data);

    // Verify popped data
    ASSERT_EQ(output_data.size(), static_cast<size_t>(5));
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(output_data[i], i);
    }
    EXPECT_TRUE(queue_->empty());
}

// Concurrency tests
TEST_F(SPSCQueueTest, BasicConcurrency) {
    CreateQueue<int>(1000);

    constexpr int NUM_ITEMS = 100000;
    std::atomic<bool> producer_done{false};
    std::vector<int> consumed_items;
    consumed_items.reserve(NUM_ITEMS);

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            while (!queue_->Emplace(i)) {
                std::this_thread::yield();
            }
        }
        producer_done = true;
    });

    // Consumer thread
    std::thread consumer([&]() {
        int value;
        while (consumed_items.size() < NUM_ITEMS) {
            if (queue_->Pop(value)) {
                consumed_items.push_back(value);
            } else if (producer_done.load()) {
                // Producer is done, try a few more times to drain the queue
                bool found_more = false;
                for (int i = 0; i < 10; ++i) {
                    if (queue_->Pop(value)) {
                        consumed_items.push_back(value);
                        found_more = true;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
                if (!found_more)
                    break;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    // Verify all items were consumed in order
    EXPECT_EQ(consumed_items.size(), static_cast<size_t>(NUM_ITEMS));
    for (int i = 0; i < NUM_ITEMS; ++i) {
        EXPECT_EQ(consumed_items[i], i);
    }
}

// Stress test with random operations
TEST_F(SPSCQueueTest, StressTestRandomOperations) {
    CreateQueue<int>(100);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    std::vector<int> reference_queue;

    for (int iteration = 0; iteration < 1000; ++iteration) {
        int num_ops = dis(gen);

        // Random push operations
        for (int i = 0; i < num_ops && reference_queue.size() < 100; ++i) {
            int value = iteration * 100 + i;
            if (queue_->Emplace(value)) {
                reference_queue.push_back(value);
            }
        }

        // Random pop operations
        int num_pops = dis(gen);
        for (int i = 0; i < num_pops && !reference_queue.empty(); ++i) {
            int value;
            if (queue_->Pop(value)) {
                EXPECT_EQ(value, reference_queue.front());
                reference_queue.erase(reference_queue.begin());
            }
        }

        EXPECT_EQ(queue_->size(), static_cast<size_t>(reference_queue.size()));
        EXPECT_EQ(queue_->empty(), reference_queue.empty());
    }

    // Clean up remaining elements
    int value;
    while (queue_->Pop(value)) {
        // Empty the queue
    }
}

// Performance benchmark test
TEST_F(SPSCQueueTest, PerformanceBenchmark) {
    CreateQueue<int>(10000);

    constexpr int NUM_OPERATIONS = 1000000;

    auto start = std::chrono::high_resolution_clock::now();

    // Benchmark single operations
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        queue_->Emplace(i);
        int value;
        queue_->Pop(value);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performance: " << NUM_OPERATIONS << " push/pop pairs in " << duration.count()
              << " microseconds" << std::endl;
    std::cout << "Operations per second: " << (NUM_OPERATIONS * 2 * 1000000.0) / duration.count()
              << std::endl;

    // Just verify the test ran (no specific performance requirements)
    EXPECT_GT(duration.count(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
