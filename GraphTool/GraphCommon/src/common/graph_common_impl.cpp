module;
#include <pch.h>

module graph_common;

static constexpr std::string_view reset = "\x1b[0m";
static constexpr std::string_view gray = "\x1b[90m";
static constexpr std::string_view light_gray = "\x1b[37m";
static constexpr std::string_view yellow = "\x1b[33m";
static constexpr std::string_view red = "\x1b[31m";

namespace common {
    Logger& Logger::get() {
        static Logger logger;
        return logger;
    }

    void Logger::setLevel(Level level) { m_level = level; }

    void Logger::logUnformatted(Level level, const char* message) {
        if (m_level > level) {
            return;
        }

#ifndef __EMSCRIPTEN__
        std::cout << getColor(level);
#endif

        logTimestamp();
        logLevel(level);

        std::cout << message << '\n';
    }

    std::string_view Logger::getColor(Level level) const {
        switch (level) {
            case Level::DEBUG_LEVEL:
                return gray;
            case Level::INFORMATION_LEVEL:
                return light_gray;
            case Level::WARNING_LEVEL:
                return yellow;
            case Level::ERROR_LEVEL:
                return red;
            default:
                return reset;
        }
    }

    void Logger::logTimestamp() {
        using namespace std::chrono;

        const auto now = system_clock::to_time_t(system_clock::now());

        std::tm localTime;

#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        std::cout << '[' << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << ']';
    }

    void Logger::logLevel(Level level) {
        const std::string_view levelString = [level]() {
            switch (level) {
                case Level::DEBUG_LEVEL:
                    return "[debug]";
                case Level::INFORMATION_LEVEL:
                    return "[information]";
                case Level::WARNING_LEVEL:
                    return "[warning]";
                case Level::ERROR_LEVEL:
                    return "[error]";
                default:
                    return "[]";
            }
        }();

        std::cout << ' ' << levelString << ' ';
    }

    ScopedTimer::~ScopedTimer() {
        using namespace std::chrono;

        const auto duration = duration_cast<milliseconds>(steady_clock::now() - m_start).count();
        common::Logger::get().information("{} took {} ms.", m_name, duration);
    }
}  // namespace common
