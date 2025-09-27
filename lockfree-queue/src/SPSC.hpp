/*
 * Portions of this code are licensed under the Apache License, Version 2.0
 * Original source: https://radiantsoftware.hashnode.dev/c-lock-free-queue-part-i
 * Copyright 2023 Paul Mattione
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.hpp"
#include <algorithm>
#include <atomic>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <span>

template <typename DataType, WaitPolicy Waiting>
class Queue<DataType, ThreadsPolicy::SPSC, Waiting> {
    // CONSTEXPR MEMBERS (needed for requires clauses)
    static constexpr bool sPushAwait = Await_Pushes(Waiting);
    static constexpr bool sPopAwait = Await_Pops(Waiting);
    static constexpr int sSizeMask = 0x80000000;  // ASSUMES 32 BIT int!
    static constexpr auto sAlign = hardware_destructive_interference_size;

  public:
    // Constructor/Destructor
    Queue() = default;
    ~Queue() = default;

    // Memory management

    template <typename AllocatorType>
    void Allocate(AllocatorType& aAllocator, int aCapacity) {
        Assert(!Is_Allocated(), "Can't allocate while still owning memory!\n");
        Assert(aCapacity > 0, "Invalid capacity {}!\n", aCapacity);

        // Allocate memory for object storage
        auto cNumBytes = aCapacity * sizeof(DataType);
        static constexpr auto sAlignment = std::max(sAlign, alignof(DataType));
        mStorage = aAllocator.Allocate(cNumBytes, sAlignment);
        Assert(mStorage != nullptr, "Memory allocation failed!\n");
        mCapacity = aCapacity;

        // Calculate where index values will wrap-around to zero
        static constexpr auto sMaxValue = std::numeric_limits<int>::max();
        auto cMaxNumWrapArounds = sMaxValue / mCapacity;
        Assert(cMaxNumWrapArounds >= 2, "Not enough wrap-arounds!\n");
        mIndexEnd = mCapacity * cMaxNumWrapArounds;
    }

    bool Is_Allocated() const {
        return (mStorage != nullptr);
    }

    template <typename AllocatorType>
    void Free(AllocatorType& aAllocator) {
        Assert(Is_Allocated(), "No memory to free!\n");
        Assert(empty(), "Can't free until empty!\n");

        aAllocator.Free(mStorage);
        mStorage = nullptr;
        mCapacity = 0;
    }

    template <typename... ArgumentTypes>
    bool Emplace(ArgumentTypes&&... aArguments) {
        // Load indices
        // Push load relaxed: Only this thread can modify it
        auto cUnwrappedPushIndex = mPushIndex.value.load(std::memory_order::relaxed);
        // Pop load acquire: Object creation cannot be reordered above this
        auto cUnwrappedPopIndex = mPopIndex.value.load(std::memory_order::acquire);

        // Guard against the container being full
        auto cIndexDelta = cUnwrappedPushIndex - cUnwrappedPopIndex;
        if ((cIndexDelta == mCapacity) || (cIndexDelta == (mCapacity - mIndexEnd)))
            return false;  // Full. The second check handled wrap-around

        // Emplace the object
        auto cPushIndex = cUnwrappedPushIndex % mCapacity;
        auto cAddress = mStorage + cPushIndex * sizeof(DataType);
        new (cAddress) DataType(std::forward<ArgumentTypes>(aArguments)...);

        // Advance push index
        auto cNewPushIndex = Bump_Index(cUnwrappedPushIndex);
        // Push store release: Object creation cannot be reordered below this
        mPushIndex.value.store(cNewPushIndex, std::memory_order::release);

        // Update the size
        Increase_Size(1);
        return true;
    }

    bool Pop(DataType& aPopped) {
        // Load indices
        // Push load acquire: The pop cannot be reordered above this
        auto cUnwrappedPushIndex = mPushIndex.value.load(std::memory_order::acquire);
        // Pop load relaxed: Only this thread can modify it
        auto cUnwrappedPopIndex = mPopIndex.value.load(std::memory_order::relaxed);

        // Guard against the container being empty
        if (cUnwrappedPopIndex == cUnwrappedPushIndex)
            return false;  // The queue is empty

        // Pop data
        auto cPopIndex = cUnwrappedPopIndex % mCapacity;
        auto cAddress = mStorage + cPopIndex * sizeof(DataType);
        auto cData = std::launder(reinterpret_cast<DataType*>(cAddress));
        aPopped = std::move(*cData);
        cData->~DataType();

        // Advance pop index
        auto cNewPopIndex = Bump_Index(cUnwrappedPopIndex);
        // Pop store release: The pop cannot be reordered below this
        mPopIndex.value.store(cNewPopIndex, std::memory_order::release);

        // Update the size
        Decrease_Size(1);
        return true;
    }

    template <typename InputType>
    std::span<InputType> Emplace_Multiple(const std::span<InputType>& aSpan) {
        // Load indices
        // Push load relaxed: Only this thread can modify it
        auto cUnwrappedPushIndex = mPushIndex.value.load(std::memory_order::relaxed);
        // Pop load acquire: Object creation cannot be reordered above this
        auto cUnwrappedPopIndex = mPopIndex.value.load(std::memory_order::acquire);

        // Can only push up to the pop index
        auto cMaxPushIndex = cUnwrappedPopIndex + mCapacity;
        auto cMaxSlotsAvailable = cMaxPushIndex - cUnwrappedPushIndex;
        cMaxSlotsAvailable -= (cMaxSlotsAvailable >= mIndexEnd) ? mIndexEnd : 0;
        const auto cSpanSize = static_cast<int>(aSpan.size());
        auto cNumToPush = std::min(cSpanSize, cMaxSlotsAvailable);
        if (cNumToPush == 0)
            return aSpan;  // The queue is full.

        // Setup push
        auto cPushIndex = cUnwrappedPushIndex % mCapacity;
        auto cPushAddress = mStorage + (cPushIndex * sizeof(DataType));
        auto cPushToData = std::launder(reinterpret_cast<DataType*>(cPushAddress));
        auto cDistanceBeyondEnd = (cPushIndex + cNumToPush) - mCapacity;
        const auto cSpanData = aSpan.data();

        // Push data (if const input just copies, else moves)
        if (cDistanceBeyondEnd <= 0)
            std::uninitialized_move_n(cSpanData, cNumToPush, cPushToData);
        else {
            auto cInitialLength = cNumToPush - cDistanceBeyondEnd;
            std::uninitialized_move_n(cSpanData, cInitialLength, cPushToData);

            cPushToData = std::launder(reinterpret_cast<DataType*>(mStorage));
            auto cToPush = cSpanData + cInitialLength;
            std::uninitialized_move_n(cToPush, cDistanceBeyondEnd, cPushToData);
        }

        // Advance push index
        auto cNewPushIndex = Increase_Index(cUnwrappedPushIndex, cNumToPush);
        // Push store release: Object creation cannot be reordered below this
        mPushIndex.value.store(cNewPushIndex, std::memory_order::release);

        // Update the size
        Increase_Size(cNumToPush);

        // Return unfinished entries
        auto cRemainingBegin = cSpanData + cNumToPush;
        return std::span<InputType>(cRemainingBegin, cSpanSize - cNumToPush);
    }

    template <typename ContainerType>
    void Pop_Multiple(ContainerType& aPopped) {
        // Load indices
        // Push load acquire: The pop cannot be reordered above this
        auto cUnwrappedPushIndex = mPushIndex.value.load(std::memory_order::acquire);
        // Pop load relaxed: Only this thread can modify it
        auto cUnwrappedPopIndex = mPopIndex.value.load(std::memory_order::relaxed);

        // Can only pop up to the push index
        auto cMaxSlotsAvailable = cUnwrappedPushIndex - cUnwrappedPopIndex;
        cMaxSlotsAvailable += (cMaxSlotsAvailable < 0) ? mIndexEnd : 0;
        auto cOutputSpaceAvailable = static_cast<int>(aPopped.capacity() - aPopped.size());
        auto cNumToPop = std::min(cOutputSpaceAvailable, cMaxSlotsAvailable);
        if (cNumToPop == 0)
            return;  // The queue is empty.

        // Helper function for Pop/destroy
        auto cPopAndDestroy = [&](DataType* aPopData, int aNumToPop) {
            // Pop data
            aPopped.insert(std::end(aPopped), std::move_iterator(aPopData),
                           std::move_iterator(aPopData + aNumToPop));

            // Destroy old data
            std::destroy_n(aPopData, aNumToPop);
        };

        // Setup pop/destroy
        auto cPopIndex = cUnwrappedPopIndex % mCapacity;
        auto cPopAddress = mStorage + (cPopIndex * sizeof(DataType));
        auto cPopFromData = std::launder(reinterpret_cast<DataType*>(cPopAddress));
        auto cDistanceBeyondEnd = (cPopIndex + cNumToPop) - mCapacity;

        // Push data (if const input just copies, else moves)
        if (cDistanceBeyondEnd <= 0)
            cPopAndDestroy(cPopFromData, cNumToPop);
        else {
            auto cInitialLength = cNumToPop - cDistanceBeyondEnd;
            cPopAndDestroy(cPopFromData, cInitialLength);

            cPopFromData = std::launder(reinterpret_cast<DataType*>(mStorage));
            cPopAndDestroy(cPopFromData, cDistanceBeyondEnd);
        }

        // Advance pop index
        auto cNewPopIndex = Increase_Index(cUnwrappedPopIndex, cNumToPop);
        // Pop store release: The pop cannot be reordered below this
        mPopIndex.value.store(cNewPopIndex, std::memory_order::release);

        // Update the size
        Decrease_Size(cNumToPop);
    }

    template <typename... ArgumentTypes>
    void Emplace_Await(ArgumentTypes&&... aArguments) requires(sPushAwait) {
        // Acquire: Need sync to see the latest queue indices
        while (!Emplace(std::forward<ArgumentTypes>(aArguments)...))
            mSize.value.wait(mCapacity, std::memory_order::acquire);
    }

    template <typename InputType>
    void Emplace_Multiple_Await(std::span<InputType> aSpan) requires(sPushAwait) {
        while (true) {
            aSpan = Emplace_Multiple(aSpan);
            if (aSpan.empty())
                return;

            // Acquire: Need sync to see the latest queue indices
            mSize.value.wait(mCapacity, std::memory_order::acquire);
        }
    }

    bool Pop_Await(DataType& aPopped) requires(sPopAwait) {
        while (true) {
            if (Pop(aPopped))
                return true;

            // The queue was empty, wait until someone pushes or we're ending
            // Acquire: Need sync to see the latest queue indices
            mSize.value.wait(0, std::memory_order::acquire);

            // If mSize is sSizeMask then nothing will push, and none left to pop.
            // Relaxed: Nothing to sync, and if we miss sSizeMask we'll see it soon
            if (mSize.value.load(std::memory_order::relaxed) == sSizeMask)
                return false;
        }
    }

    template <typename ContainerType>
    void Pop_Multiple_Await(ContainerType& aPopped) requires(sPopAwait) {
        while (true) {
            Pop_Multiple(aPopped);
            if (!aPopped.empty())
                return;

            // Comments are identical to Pop_Await()
            mSize.value.wait(0, std::memory_order::relaxed);

            // If mSize is sSizeMask then nothing will push, and none left to pop.
            // Relaxed: Nothing to sync, and if we miss sSizeMask we'll see it soon
            if (mSize.value.load(std::memory_order::relaxed) == sSizeMask)
                return;
        }
    }

    // Queue state
    size_t size() const {
        // Relaxed: Nothing to synchronize when reading this
        auto cSize = mSize.value.load(std::memory_order::relaxed);
        return cSize & (~sSizeMask);  // Clear the high bit!
    }

    bool empty() const {
        return size() == 0;
    }

    // Wait control
    void End_PopWaiting() requires(sPopAwait) {
        // Tell all popping threads we're shutting down
        // Release order: Syncs indices, and prevents code reordering after this.
        auto cPriorSize = mSize.value.fetch_or(sSizeMask, std::memory_order::release);

        // Notify any waiting threads, but only if it was empty
        if (cPriorSize == 0)
            mSize.value.notify_all();
    }

    void Reset_PopWaiting() requires(sPopAwait) {
        // Relaxed: Sync of other data is not needed: Queue state unchanged
        mSize.value.fetch_and(~sSizeMask, std::memory_order::relaxed);
    }

  private:
    // Helper methods
    int Bump_Index(int aIndex) const {
        int cIncremented = aIndex + 1;
        return (cIncremented < mIndexEnd) ? cIncremented : 0;
    }

    int Increase_Index(int aIndex, int aIncrease) const {
        int cNewIndex = aIndex + aIncrease;
        cNewIndex -= (cNewIndex >= mIndexEnd) ? mIndexEnd : 0;
        return cNewIndex;
    }

    void Increase_Size(int aNumPushed) {
        // Release if pop-awaiting (Syncs indices), else relaxed (no sync needed)
        static constexpr auto sOrder =
            sPopAwait ? std::memory_order::release : std::memory_order::relaxed;
        [[maybe_unused]] auto cPriorSize = mSize.value.fetch_add(aNumPushed, sOrder);

        if constexpr (sPopAwait) {
            // If was empty, notify all threads
            // No need to clear high bit: if set, pop-waits already ended
            if (cPriorSize == 0)
                mSize.value.notify_all();
        }
    }

    void Decrease_Size(int aNumPopped) {
        // Release if push-awaiting (Syncs indices), else relaxed (no sync needed)
        static constexpr auto sOrder =
            sPushAwait ? std::memory_order::release : std::memory_order::relaxed;
        [[maybe_unused]] auto cPriorSize = mSize.value.fetch_sub(aNumPopped, sOrder);

        if constexpr (sPushAwait) {
            // If was full (clear the high bit!), notify all threads
            if ((cPriorSize & (~sSizeMask)) == mCapacity)
                mSize.value.notify_all();
        }
    }

    // OVER-ALIGNED MEMBERS
    struct alignas(sAlign) PaddedAtomicInt {
        std::atomic<int> value{0};
    };

    PaddedAtomicInt mPushIndex;
    PaddedAtomicInt mPopIndex;
    PaddedAtomicInt mSize;

    // DEFAULT-ALIGNED MEMBERS
    // Not over-aligned as neither these nor the mStorage pointer change
    std::byte* mStorage = nullptr;  // Object Memory
    int mCapacity = 0;
    int mIndexEnd = 0;  // at this, we need to wrap indices around to zero
};

template <typename DataType, WaitPolicy Waiting>
using SPSC = Queue<DataType, ThreadsPolicy::SPSC, Waiting>;