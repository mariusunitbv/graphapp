module;
#include <pch.h>

export module log_view;

export import graph_common;

export class LogView : public common::LogListener {
   public:
    void render();

    bool& isOpen();

   protected:
    void onLogMessage(common::Logger::Level level, const std::string_view message) override;

   private:
    struct LineData {
        explicit LineData(uint32_t offset, uint32_t level)
            : m_lineOffset(offset), m_lineLevel(level) {}

        uint32_t m_lineOffset : 29;
        uint32_t m_lineLevel : 3;
    };

    bool m_isOpen{true};

    std::vector<char> m_logBuffer;
    std::vector<LineData> m_logLines;
    ImGuiTextFilter m_logFilter;
    mutable std::shared_mutex m_logMutex;
};
