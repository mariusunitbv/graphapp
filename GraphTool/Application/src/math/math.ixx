module;
#include <pch.h>

export module math;

export struct Vector2D {
    Vector2D() = default;
    Vector2D(float x, float y) : m_x(x), m_y(y) {}

    auto distanceSquared(Vector2D other) const {
        const auto dx = m_x - other.m_x;
        const auto dy = m_y - other.m_y;
        return dx * dx + dy * dy;
    }

    auto operator-(Vector2D other) const { return Vector2D(m_x - other.m_x, m_y - other.m_y); }
    auto operator+(Vector2D other) const { return Vector2D(m_x + other.m_x, m_y + other.m_y); }
    auto operator*(float scalar) const { return Vector2D(m_x * scalar, m_y * scalar); }
    auto operator/(float scalar) const { return Vector2D(m_x / scalar, m_y / scalar); }

    auto operator-() const { return Vector2D(-m_x, -m_y); }

    float m_x{0.f}, m_y{0.f};
};

export struct BoundingBox2D {
    BoundingBox2D() = default;

    BoundingBox2D(float left, float top, float right, float bottom)
        : m_min(left, top), m_max(right, bottom) {}
    BoundingBox2D(Vector2D min, Vector2D max) : m_min(min), m_max(max) {}

    auto width() const { return m_max.m_x - m_min.m_x; }
    auto height() const { return m_max.m_y - m_min.m_y; }

    auto intersects(const BoundingBox2D& other) const {
        return m_min.m_x < other.m_max.m_x && m_max.m_x > other.m_min.m_x &&
               m_min.m_y < other.m_max.m_y && m_max.m_y > other.m_min.m_y;
    }

    auto contains(BoundingBox2D other) const {
        return m_min.m_x <= other.m_min.m_x && m_max.m_x >= other.m_max.m_x &&
               m_min.m_y <= other.m_min.m_y && m_max.m_y >= other.m_max.m_y;
    }

    Vector2D m_min{};
    Vector2D m_max{};
};
