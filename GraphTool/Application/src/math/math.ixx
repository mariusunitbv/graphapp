module;
#include <pch.h>

export module math;

export struct Vector2D {
    constexpr Vector2D() = default;
    constexpr Vector2D(float x, float y) : m_x(x), m_y(y) {}

    static constexpr auto min(Vector2D a, Vector2D b) {
        return Vector2D(std::min(a.m_x, b.m_x), std::min(a.m_y, b.m_y));
    }

    static constexpr auto max(Vector2D a, Vector2D b) {
        return Vector2D(std::max(a.m_x, b.m_x), std::max(a.m_y, b.m_y));
    }

    constexpr auto distanceSquared(Vector2D other) const {
        const auto dx = m_x - other.m_x;
        const auto dy = m_y - other.m_y;
        return dx * dx + dy * dy;
    }

    constexpr auto operator-(Vector2D other) const {
        return Vector2D(m_x - other.m_x, m_y - other.m_y);
    }
    constexpr auto operator+(Vector2D other) const {
        return Vector2D(m_x + other.m_x, m_y + other.m_y);
    }
    constexpr auto operator*(float scalar) const { return Vector2D(m_x * scalar, m_y * scalar); }
    constexpr auto operator/(float scalar) const { return Vector2D(m_x / scalar, m_y / scalar); }

    constexpr auto operator-() const { return Vector2D(-m_x, -m_y); }

    float m_x{0.f}, m_y{0.f};
};

export struct BoundingBox2D {
    constexpr BoundingBox2D() = default;

    constexpr BoundingBox2D(float left, float top, float right, float bottom)
        : m_min(left, top), m_max(right, bottom) {}
    constexpr BoundingBox2D(Vector2D min, Vector2D max) : m_min(min), m_max(max) {}

    constexpr auto width() const { return m_max.m_x - m_min.m_x; }
    constexpr auto height() const { return m_max.m_y - m_min.m_y; }

    constexpr auto valid() const { return m_min.m_x < m_max.m_x && m_min.m_y < m_max.m_y; }

    constexpr auto intersects(const BoundingBox2D& other) const {
        return m_min.m_x < other.m_max.m_x && m_max.m_x > other.m_min.m_x &&
               m_min.m_y < other.m_max.m_y && m_max.m_y > other.m_min.m_y;
    }

    constexpr auto contains(const BoundingBox2D& other) const {
        return m_min.m_x <= other.m_min.m_x && m_max.m_x >= other.m_max.m_x &&
               m_min.m_y <= other.m_min.m_y && m_max.m_y >= other.m_max.m_y;
    }

    constexpr auto contains(Vector2D point) const {
        return m_min.m_x <= point.m_x && m_max.m_x >= point.m_x && m_min.m_y <= point.m_y &&
               m_max.m_y >= point.m_y;
    }

    constexpr BoundingBox2D& clamp(const BoundingBox2D& bounds) {
        m_min = Vector2D::max(m_min, bounds.m_min);
        m_max = Vector2D::min(m_max, bounds.m_max);
        return *this;
    }

    Vector2D m_min{};
    Vector2D m_max{};
};

export constexpr ImVec2 toImVec(Vector2D vec) { return ImVec2(vec.m_x, vec.m_y); }
