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

    Logger::Logger() { m_messageBuffer.reserve(150); }

    void Logger::setLevel(Level level) { m_level = level; }

    void Logger::logUnformatted(Level level, const char* message) {
        if (m_level > level) {
            return;
        }

        m_messageBuffer.clear();

        size_t positionWithoutColor = 0;

#ifndef __EMSCRIPTEN__
        m_messageBuffer.append(getColor(level));
        positionWithoutColor = m_messageBuffer.size();
#endif

        addTimestampToBuffer();
        addLevelToBuffer(level);

        m_messageBuffer.append(message);
        m_messageBuffer.push_back('\n');

        std::cout << m_messageBuffer;

        for (LogListener* listener : m_listeners) {
            listener->onLogMessage(level,
                                   std::string_view(m_messageBuffer).substr(positionWithoutColor));
        }
    }

    void Logger::addListener(LogListener* listener) { m_listeners.push_back(listener); }

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

    void Logger::addTimestampToBuffer() {
        const auto now = std::chrono::system_clock::now();
        const auto nowSec = std::chrono::floor<std::chrono::seconds>(now);

        m_messageBuffer.append(std::format("[{:%Y-%m-%d %H:%M:%S}]", nowSec));
    }

    void Logger::addLevelToBuffer(Level level) {
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

        m_messageBuffer.push_back(' ');
        m_messageBuffer.append(levelString);
        m_messageBuffer.push_back(' ');
    }

    ScopedTimer::~ScopedTimer() {
        using namespace std::chrono;

        const auto duration = duration_cast<milliseconds>(steady_clock::now() - m_start).count();
        common::Logger::get().information("{} took {} ms.", m_name, duration);
    }
}  // namespace common

namespace common {
    FileSystem& FileSystem::get() {
        static FileSystem fileSystem;
        return fileSystem;
    }

    void FileSystem::createFolder(const std::string& path) const {
        Logger::get().debug("Creating folder: {}", path);
        std::filesystem::create_directories(path);
    }
}  // namespace common
