module;
#include <pch.h>

export module graph_common;

export namespace common {
#pragma pack(push, 1)
    template <typename T, typename S>
    class Vector {
       public:
        Vector() = default;
        ~Vector() { destruct(); }

        Vector(S initialSize) : m_size(initialSize), m_capacity(initialSize) {
            if (initialSize == 0) {
                m_data = nullptr;
                return;
            }

            m_data = static_cast<T*>(::operator new(m_capacity * sizeof(T)));

            if constexpr (!std::is_trivially_constructible_v<T>) {
                for (S i = 0; i < m_size; ++i) {
                    new (m_data + i) T{};
                }
            }
        }

        Vector(const Vector& other) : m_size(other.m_size), m_capacity(other.m_capacity) {
            if (m_capacity == 0) {
                m_data = nullptr;
                return;
            }

            m_data = static_cast<T*>(::operator new(m_capacity * sizeof(T)));
            if (std::is_trivially_copyable_v<T>) {
                std::memcpy(m_data, other.m_data, m_size * sizeof(T));
            } else {
                for (S i = 0; i < m_size; ++i) {
                    new (m_data + i) T(other.m_data[i]);
                }
            }
        }

        Vector(Vector&& other) noexcept
            : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity) {
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        Vector& operator=(const Vector& other) {
            if (this != &other) {
                destruct();

                m_size = other.m_size;
                m_capacity = other.m_capacity;

                if (m_capacity == 0) {
                    m_data = nullptr;
                    return;
                }

                m_data = static_cast<T*>(::operator new(m_capacity * sizeof(T)));
                if (std::is_trivially_copyable_v<T>) {
                    std::memcpy(m_data, other.m_data, m_size * sizeof(T));
                } else {
                    for (S i = 0; i < m_size; ++i) {
                        new (m_data + i) T(other.m_data[i]);
                    }
                }
            }

            return *this;
        }

        Vector& operator=(Vector&& other) noexcept {
            if (this != &other) {
                destruct();

                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;

                other.m_data = nullptr;
                other.m_size = 0;
                other.m_capacity = 0;
            }

            return *this;
        }

        bool empty() const { return m_size == 0; }

        auto begin() { return m_data; }
        auto begin() const { return m_data; }

        auto end() { return m_data + m_size; }
        auto end() const { return m_data + m_size; }

        T& operator[](S index) { return m_data[index]; }
        const T& operator[](S index) const { return m_data[index]; }

        T& back() { return m_data[m_size - 1]; }
        const T& back() const { return m_data[m_size - 1]; }

        T* data() { return m_data; }
        const T* data() const { return m_data; }

        auto size() const { return m_size; }
        void clear() {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (S i = 0; i < m_size; ++i) {
                    m_data[i].~T();
                }
            }

            m_size = 0;
        }

        void shrink_to_fit() {
            if (m_size < m_capacity) {
                grow(m_size);
            }
        }

        void reserve(S newCapacity) {
            if (newCapacity <= m_capacity) {
                return;
            }

            grow(newCapacity);
        }

        void resize(S newSize, const T& defaultValue = T{}) {
            if (newSize > m_capacity) {
                grow(newSize);
            }

            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (S i = newSize; i < m_size; ++i) {
                    m_data[i].~T();
                }
            }

            for (S i = m_size; i < newSize; ++i) {
                new (m_data + i) T{defaultValue};
            }

            m_size = newSize;
        }

        void push_back(const T& value) {
            if (m_size == m_capacity) {
                grow();
            }

            new (m_data + m_size) T(value);
            ++m_size;
        }

        void push_back(T&& value) {
            if (m_size == m_capacity) {
                grow();
            }

            new (m_data + m_size) T(std::move(value));
            ++m_size;
        }

        template <typename... Args>
        T& emplace_back(Args&&... args) {
            if (m_size == m_capacity) {
                grow();
            }

            new (m_data + m_size) T(std::forward<Args>(args)...);
            return m_data[m_size++];
        }

        template <typename... Args>
        T& insert(T* position, Args&&... args) {
            S index = static_cast<S>(position - m_data);

            if (m_size == m_capacity) {
                grow();
            }

            for (S i = m_size; i > index; --i) {
                if (i == m_size) {
                    new (m_data + i) T(std::move_if_noexcept(m_data[i - 1]));
                } else {
                    m_data[i] = std::move_if_noexcept(m_data[i - 1]);
                }
            }

            if constexpr (!std::is_trivially_destructible_v<T>) {
                if (index < m_size) {
                    m_data[index].~T();
                }
            }

            ++m_size;

            new (m_data + index) T(std::forward<Args>(args)...);
            return m_data[index];
        }

        void pop_back() {
            if (m_size > 0) {
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    m_data[m_size - 1].~T();
                }

                --m_size;
            }
        }

        T* erase(T* position) {
            S index = static_cast<S>(position - m_data);
            for (S i = index; i < m_size - 1; ++i) {
                m_data[i] = std::move_if_noexcept(m_data[i + 1]);
            }

            if constexpr (!std::is_trivially_destructible_v<T>) {
                m_data[m_size - 1].~T();
            }

            --m_size;
            return m_data + index;
        }

        std::span<T> span() { return {m_data, m_size}; }
        std::span<const T> span() const { return {m_data, m_size}; }

       private:
        void grow() { reallocate(m_capacity > 0 ? m_capacity * 2 : 1); }
        void grow(S newCapacity) { reallocate(newCapacity); }

        void reallocate(S newCapacity) {
            T* newData = static_cast<T*>(::operator new(newCapacity * sizeof(T)));

            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(newData, m_data, m_size * sizeof(T));
            } else {
                for (S i = 0; i < m_size; ++i) {
                    new (newData + i) T(std::move_if_noexcept(m_data[i]));
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        m_data[i].~T();
                    }
                }
            }

            ::operator delete(m_data);

            m_data = newData;
            m_capacity = newCapacity;
        }

        void destruct() {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (S i = 0; i < m_size; ++i) {
                    m_data[i].~T();
                }
            }

            ::operator delete(m_data);
        }

        T* m_data{nullptr};
        S m_size{0};
        S m_capacity{0};
    };
#pragma pack(pop)

    template <typename T>
    using TinyVector = Vector<T, uint8_t>;

    template <typename T>
    using SmallVector = Vector<T, uint16_t>;

    template <typename T>
    using MediumVector = Vector<T, uint32_t>;

    template <typename T>
    using LargeVector = Vector<T, uint64_t>;

    static_assert(sizeof(TinyVector<int>) == sizeof(void*) + sizeof(uint8_t) * 2);
    static_assert(sizeof(SmallVector<int>) == sizeof(void*) + sizeof(uint16_t) * 2);
    static_assert(sizeof(MediumVector<int>) == sizeof(void*) + sizeof(uint32_t) * 2);
    static_assert(sizeof(LargeVector<int>) == sizeof(void*) + sizeof(uint64_t) * 2);

    namespace algorithms {
        template <typename T, typename S, typename Pred>
        S erase_if(Vector<T, S>& vec, Pred pred) {
            S writeIndex = 0, oldSize = vec.size();
            for (S readIndex = 0; readIndex < oldSize; ++readIndex) {
                if (pred(vec[readIndex])) {
                    continue;
                }

                if (writeIndex != readIndex) {
                    vec[writeIndex] = std::move_if_noexcept(vec[readIndex]);
                }

                ++writeIndex;
            }

            vec.resize(writeIndex);
            return oldSize - writeIndex;
        }
    }  // namespace algorithms
}  // namespace common
