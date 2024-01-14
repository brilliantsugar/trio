#pragma once

#if defined(TRIO_USE_RELACY)
    #include <relacy/relacy.hpp>
#else
    #include <atomic>
    #include <thread>
#endif

#if defined(TRIO_USE_MUTEX)
    #include <mutex>
#endif

#include <utility>


namespace bs {

// Lock-free single producer-single consumer triple buffer
//
// Each buffer can be of any type T
//
// if TRIO_USE_RELACY is defined: will instrument code with relacy hooks and can be used in relacy unit tests
// if TRIO_USE_MUTEX is defined: will use std::mutex instead of atomics
template<typename T>
class Trio {

    using DataType = T;

#if defined(TRIO_USE_RELACY)
    template<class A>
    using AtomicType = rl::atomic<A>;
    static constexpr auto kRelaxed = rl::mo_relaxed;
    static constexpr auto kAcqRel = rl::mo_acq_rel;
    #define TRIO_VAR(name) name($)
#else
    template<class A>
    using AtomicType = std::atomic<A>;
    static constexpr auto kRelaxed = std::memory_order_relaxed;
    static constexpr auto kAcqRel = std::memory_order_acq_rel;
    #define TRIO_VAR(name) name
#endif


public:
    // All buffers will be default initialized.
    Trio() = default;

    // Initializes front buffer with given data, other buffers are default initialized
    Trio(DataType&& initial_front_buf)
        : buffers_{{}, {}, {std::forward<DataType>(initial_front_buf)}}
    {}

    // Initializes all 3 buffers
    Trio(DataType&& initial_back_buf,
         DataType&& initial_middle_buf,
         DataType&& initial_front_buf)
        : buffers_{
            {std::forward<DataType>(initial_back_buf)},
            {std::forward<DataType>(initial_middle_buf)},
            {std::forward<DataType>(initial_front_buf)}}
    {}

#if !defined(TRIO_USE_MUTEX)
    // Consumer API
    //
    // Returns reference to the latest front buffer.
    // If there's a new buffer recently written -> will swap buffers and return the latest value.
    // Otherwise, will return stale value.
    std::pair<DataType&, bool>
    Read() {
        uintptr_t dirty_ptr = TRIO_VAR(middle_buffer_).load(kRelaxed);
        if ((dirty_ptr & kDirtyBit) == 0) {
            return {*front_buffer_, false};
        }
        uintptr_t prev = TRIO_VAR(middle_buffer_).exchange(reinterpret_cast<uintptr_t>(front_buffer_), kAcqRel);
        front_buffer_ = reinterpret_cast<DataType*>(prev & kDirtyBitMask);
        return {*front_buffer_, true};
    }

    // Producer API
    //
    // Returns reference to back buffer, producer can use it to fill the buffer.
    // Once finished filling the buffer Commit() must be called to propagate changes
    // to the front buffer.
    DataType& Write() {
        return *back_buffer_;
    }

    // Producer API
    //
    // Propagates pending changes from the back buffer to the middle buffer.
    // The next read will get those changes unless the writer is faster and will overwrite
    // those changes before the next Read() call.
    void Commit() {
        const uintptr_t dirty_ptr = kDirtyBit | reinterpret_cast<uintptr_t>(back_buffer_); 
        uintptr_t prev = TRIO_VAR(middle_buffer_).exchange(dirty_ptr, kAcqRel) & kDirtyBitMask;
        back_buffer_ = reinterpret_cast<DataType*>(prev);
    }
#else // TRIO_USE_MUTEX
    std::pair<DataType&, bool>
    Read() {
        std::lock_guard<std::mutex> guard(mutex_);
        bool dirty = dirty_;
        if (dirty) {
            std::swap(front_buffer_, middle_buffer_);
            dirty_ = false;
        }
        return {*front_buffer_, dirty};
    }

    DataType& Write() {
        return *back_buffer_;
    }

    void Commit() {
        std::lock_guard<std::mutex> guard(mutex_);
        std::swap(back_buffer_, middle_buffer_);
        dirty_ = true;
    }
#endif

private:
    static constexpr size_t kNoSharing = 64;
    static constexpr uintptr_t kDirtyBit = 1;
    static constexpr uintptr_t kDirtyBitMask = ~uintptr_t() ^ kDirtyBit;

    struct alignas(kNoSharing) Buffer {
        DataType data{};
    };

    Buffer buffers_[3];

#if !defined(TRIO_USE_MUTEX)
    AtomicType<uintptr_t> middle_buffer_{reinterpret_cast<uintptr_t>(&buffers_[1].data)};
#else
    std::mutex mutex_;
    alignas(kNoSharing)
    DataType* middle_buffer_{&buffers_[1].data};
    bool dirty_ = false;
#endif

    alignas(kNoSharing)
    DataType* back_buffer_{&buffers_[0].data};

    alignas(kNoSharing)
    DataType* front_buffer_{&buffers_[2].data};
};
} // brilliantsugar
