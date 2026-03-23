module;
#include <pch.h>

module log_view;

void LogView::render() {
    if (!m_isOpen) {
        return;
    }

    const auto& style = ImGui::GetStyle();
    const ImVec4 logLUT[] = {style.Colors[ImGuiCol_TextDisabled],  // DEBUG_LEVEL
                             style.Colors[ImGuiCol_Text],          // INFORMATION_LEVEL
                             ImVec4(0.85f, 0.85f, 0.4f, 1.f),      // WARNING_LEVEL
                             ImVec4(1.f, 0.5f, 0.5f, 1.f),         // ERROR_LEVEL
                             ImVec4(1.f, 1.f, 1.f, 1.f)};

    if (ImGui::Begin("Logs", &m_isOpen)) {
        m_logFilter.Draw();

        if (ImGui::BeginChild("LogsChild", ImVec2(0, 0), false,
                              ImGuiWindowFlags_HorizontalScrollbar)) {
            const auto buf = m_logBuffer.data();
            const auto bufEnd = buf + m_logBuffer.size();
            if (m_logFilter.IsActive()) {
                for (size_t lineIndex = 0; lineIndex < m_logLines.size(); ++lineIndex) {
                    const auto lineStart = m_logLines[lineIndex].m_lineOffset;
                    const auto level = m_logLines[lineIndex].m_lineLevel;
                    const auto lineEnd = (lineIndex + 1 < m_logLines.size())
                                             ? m_logLines[lineIndex + 1].m_lineOffset
                                             : static_cast<int>(m_logBuffer.size());

                    if (m_logFilter.PassFilter(buf + lineStart, buf + lineEnd - 1)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, logLUT[level]);
                        ImGui::TextUnformatted(buf + lineStart, buf + lineEnd - 1);
                        ImGui::PopStyleColor();
                    }
                }
            } else {
                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(m_logLines.size()));

                while (clipper.Step()) {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                        const auto lineStart = m_logLines[i].m_lineOffset;
                        const auto level = m_logLines[i].m_lineLevel;

                        const auto lineEnd = (i + 1 < static_cast<int>(m_logLines.size()))
                                                 ? m_logLines[i + 1].m_lineOffset
                                                 : static_cast<int>(m_logBuffer.size());

                        ImGui::PushStyleColor(ImGuiCol_Text, logLUT[level]);
                        ImGui::TextUnformatted(m_logBuffer.data() + lineStart,
                                               m_logBuffer.data() + lineEnd - 1);
                        ImGui::PopStyleColor();
                    }
                }

                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

bool& LogView::isOpen() { return m_isOpen; }

void LogView::onLogMessage(common::Logger::Level level, const std::string_view message) {
    std::unique_lock lock(m_logMutex);

    auto base = m_logBuffer.size();

    m_logBuffer.insert(m_logBuffer.end(), message.begin(), message.end());
    m_logBuffer.push_back('\n');

    size_t lineStart = base;
    for (size_t i = 0; i < message.size(); ++i) {
        if (message[i] == '\n') {
            m_logLines.emplace_back(static_cast<uint32_t>(lineStart), static_cast<uint32_t>(level));

            lineStart = base + i + 1;
        }
    }
}
