#pragma once

#include <source_location>
#include <stdexcept>
#include <format>

#define GAPP_VERSION "pre1.0.1"

#ifdef __EMSCRIPTEN__
#define GAPP_THROW(message)
#else
#define GAPP_THROW(message)                                                       \
    throw std::runtime_error(                                                     \
        std::format("{}:{}\n{}\n{}", std::source_location::current().file_name(), \
                    std::source_location::current().line(),                       \
                    std::source_location::current().function_name(), message))
#endif

inline uint32_t xorshift32() noexcept {
    static uint32_t state = 0xDEADBEEF;

    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    return state = x;
}
