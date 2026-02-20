module;
#include <pch.h>

export module utils;

import math;

export inline ImVec2 toImVec(Vector2D vec) { return ImVec2(vec.m_x, vec.m_y); }

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

export class TextureLoader {
   public:
    static GLuint loadPNGFile(const char* filePath);
};
