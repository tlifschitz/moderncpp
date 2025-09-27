
class TestAllocator {
  public:
    std::byte* Allocate(size_t size, size_t alignment) {
        // Round up size to multiple of alignment (required by std::aligned_alloc)
        size_t aligned_size = ((size + alignment - 1) / alignment) * alignment;

        void* ptr = std::aligned_alloc(alignment, aligned_size);
        if (!ptr) {
            throw std::bad_alloc();
        }
        allocated_ptrs_.push_back(static_cast<std::byte*>(ptr));
        return static_cast<std::byte*>(ptr);
    }

    void Free(std::byte* ptr) {
        if (ptr) {
            auto it = std::find(allocated_ptrs_.begin(), allocated_ptrs_.end(), ptr);
            if (it != allocated_ptrs_.end()) {
                allocated_ptrs_.erase(it);
                std::free(ptr);
            }
        }
    }

    ~TestAllocator() {
        // Clean up any remaining allocations
        for (auto* ptr : allocated_ptrs_) {
            std::free(ptr);
        }
    }

    size_t allocated_count() const {
        return allocated_ptrs_.size();
    }

  private:
    std::vector<std::byte*> allocated_ptrs_;
};
