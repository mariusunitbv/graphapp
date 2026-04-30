module;
#include <pch.h>

module pseudocode_view;

import graph_common;

void PseudocodeView::loadPseudocode(AlgorithmType algorithmType) {
    m_pseudocodeLines.clear();

    const auto path = g_algorithmPseudocodes[(size_t)algorithmType];

    std::ifstream file(path.data());
    if (!file.is_open()) {
        common::Logger::get().error("Failed to load pseudocode from file: {}", path);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        PseudoCodeLine out;

        auto pos = line.find('#');
        if (pos != std::string::npos) {
            out.m_text = line.substr(0, pos);

            std::string event = line.substr(pos + 1);

            while (!event.empty() && std::isspace(event.back())) event.pop_back();
            while (!event.empty() && std::isspace(event.front())) event.erase(event.begin());

            out.m_event = event;
        } else {
            out.m_text = line;
        }

        std::transform(out.m_event.begin(), out.m_event.end(), out.m_event.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        m_pseudocodeLines.push_back(std::move(out));
    }
}

void PseudocodeView::render(const GraphModel* model, GraphViewModel* viewModel) {
    if (!m_isOpen || !viewModel->isAlgorithmCreated()) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(450, 300), ImGuiCond_FirstUseEver);

    ImGui::Begin("Pseudocode", &m_isOpen);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[m_usedFontIndex]);

    if (ImGui::Button(m_usedFontIndex == 1 ? "Increase Font" : "Decrease Font",
                      ImVec2(-FLT_MIN, 0))) {
        m_usedFontIndex = m_usedFontIndex == 1 ? 0 : 1;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, -1));
    if (m_pseudocodeLines.empty()) {
        ImGui::TextUnformatted("No pseudocode loaded.");
    } else {
        auto* draw = ImGui::GetWindowDrawList();

        const auto flashBorder =
            std::chrono::steady_clock::now() - m_lastEventTime < std::chrono::milliseconds(230);
        for (int i = 0; i < m_pseudocodeLines.size(); ++i) {
            const auto& line = m_pseudocodeLines[i];
            const auto active =
                (line.m_event == m_currentEvent && !viewModel->isAlgorithmFinished()) ||
                (line.m_event == "end" && viewModel->isAlgorithmFinished());

            ImVec2 pos = ImGui::GetCursorScreenPos();
            float lineHeight = ImGui::GetTextLineHeightWithSpacing();

            if (active) {
                draw->AddRectFilled(
                    pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + lineHeight),
                    ImGui::GetColorU32(flashBorder ? ImGuiCol_HeaderHovered : ImGuiCol_Header));
            }

            ImGui::BeginGroup();
            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%2d", i + 1);
            ImGui::SameLine();

            ImGui::PushTextWrapPos();
            if (active) {
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_Text), "%s",
                                   line.m_text.c_str());
            } else {
                ImGui::TextUnformatted(line.m_text.c_str());
            }
            ImGui::PopTextWrapPos();

#ifdef _DEBUG
            if (!line.m_event.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "#%s",
                                   line.m_event.c_str());
            }
#endif

            ImGui::EndGroup();
        }
    }
    ImGui::PopStyleVar();

    ImGui::PopFont();
    ImGui::End();
}

bool& PseudocodeView::isOpen() { return m_isOpen; }

void PseudocodeView::onAlgorithmPseudocodeEvent(const std::string_view event) {
    m_currentEvent = event;
    m_lastEventTime = std::chrono::steady_clock::now();
}
