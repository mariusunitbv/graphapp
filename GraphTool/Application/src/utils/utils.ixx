module;
#include <pch.h>

export module utils;

export template <class... Args>
struct Callback {
    using Func_t = void (*)(void*, Args...);

    void operator()(Args... args) const {
        if (m_func) {
            m_func(m_instance, std::forward<Args>(args)...);
        }
    }

    void set(void* instance, Func_t func) {
        m_instance = instance;
        m_func = func;
    }

    void* m_instance{nullptr};
    Func_t m_func{nullptr};
};

export bool intersects(const ImVec4& a, const ImVec4& b) {
    return !(a.z < b.x || a.x > b.z || a.w < b.y || a.y > b.w);
}

export bool contains(const ImVec4& area, const ImVec2& point) {
    return point.x >= area.x && point.x <= area.x + area.z && point.y >= area.y &&
           point.y <= area.y + area.w;
}

export bool contains(const ImVec4& a, const ImVec4& b) {
    return (b.x >= a.x) && (b.y >= a.y) && (b.z <= a.z) && (b.w <= a.w);
}

export float distanceSquared(const ImVec2& a, const ImVec2& b) {
    const auto dx = a.x - b.x;
    const auto dy = a.y - b.y;

    return dx * dx + dy * dy;
}

export class TextureLoader {
   public:
    static GLuint loadPNGFile(const char* filePath);
};
