#pragma once

struct MemoryTracker {
    size_t m_usedBytes{};
};

template <typename T>
class TrackingAllocator {
   public:
    using value_type = T;

    TrackingAllocator(MemoryTracker* tracker = nullptr) : m_tracker(tracker) {}

    template <typename U>
    TrackingAllocator(const TrackingAllocator<U>& other) noexcept : m_tracker(other.m_tracker) {}

    T* allocate(std::size_t n) {
        const size_t bytes = n * sizeof(T);
        if (m_tracker) {
            m_tracker->m_usedBytes += bytes;
        }

        return std::allocator<T>{}.allocate(n);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        const size_t bytes = n * sizeof(T);
        if (m_tracker) {
            m_tracker->m_usedBytes -= bytes;
        }

        std::allocator<T>{}.deallocate(p, n);
    }

    template <typename U>
    bool operator==(const TrackingAllocator<U>& other) const noexcept {
        return m_tracker == other.m_tracker;
    }

    template <typename U>
    bool operator!=(const TrackingAllocator<U>& other) const noexcept {
        return !(*this == other);
    }

    MemoryTracker* m_tracker;
};
