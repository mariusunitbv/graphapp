#pragma once

#include <source_location>
#include <stdexcept>
#include <format>

#define GAPP_VERSION "pre1.0.0"

#ifdef __EMSCRIPTEN__
#define GAPP_THROW(message)
#else
#define GAPP_THROW(message)                                                       \
    throw std::runtime_error(                                                     \
        std::format("{}:{}\n{}\n{}", std::source_location::current().file_name(), \
                    std::source_location::current().line(),                       \
                    std::source_location::current().function_name(), message))
#endif
