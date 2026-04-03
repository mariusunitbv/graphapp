module;
#include <pch.h>

#ifndef __EMSCRIPTEN__
#include <simdjson.h>
#endif

module settings_manager;

import graph_common;

void SettingsManager::initialize(GraphUI* graphUI, GraphDocumentHandler* documentHandler) {
    m_graphUI = graphUI;
    m_documentHandler = documentHandler;
}

void SettingsManager::saveSettings() {
#ifndef __EMSCRIPTEN__
    using namespace simdjson;

    if (!m_graphUI || !m_documentHandler) {
        common::Logger::get().warning(
            "SettingsManager not properly initialized, skipping saving settings.");
        return;
    }

    builder::string_builder sb;
    sb.start_object();
    {
        sb.append_key_value<"ui_theme_index">(static_cast<int>(m_graphUI->currentTheme()));
        sb.append_comma();
        sb.append_key_value<"graph_theme_index">(static_cast<int>(m_graphUI->currentGraphTheme()));
        sb.append_comma();
        sb.append_key_value<"settings_open">(m_graphUI->isSettingsOpen());
        sb.append_comma();
        sb.append_key_value<"fullscreen">(m_graphUI->isFullScreen());
        sb.append_comma();
        sb.append_key_value<"file_view_open">(m_graphUI->fileViewOpen());
        sb.append_comma();
        sb.append_key_value<"inspector_open">(m_graphUI->inspectorOpen());
        sb.append_comma();
        sb.append_key_value<"node_viewer_open">(m_graphUI->nodeViewerOpen());
        sb.append_comma();
        sb.append_key_value<"logs_open">(m_graphUI->logsOpen());
        sb.append_comma();
        sb.append_key_value<"vsync_mode">(m_graphUI->getVsyncMode());
        sb.append_comma();
        sb.append_key_value<"fps_limit_enabled">(m_graphUI->isFpsLimitEnabled());
        sb.append_comma();
        sb.append_key_value<"max_fps">(m_graphUI->getMaxFps());
        sb.append_comma();
        sb.append_key_value<"opened_root_folder">(m_graphUI->getOpenedRootFolder());
        sb.append_comma();

        sb.append_key_value<"opened_document_index">(
            m_documentHandler->getCurrentOpenedDocumentIndex());
        sb.append_comma();

        const auto& openDocuments = m_documentHandler->getOpenedDocuments();

        sb.escape_and_append_with_quotes<"graphs_paths">();
        sb.append_colon();
        sb.start_array();
        {
            for (size_t i = 0; i < openDocuments.size(); ++i) {
                if (!openDocuments[i].m_path.empty()) {
                    sb.append(openDocuments[i].m_path);
                    if (i != openDocuments.size() - 1) {
                        sb.append_comma();
                    }
                }
            }
        }
        sb.end_array();

        if (m_graphUI->currentGraphTheme() == GraphUI::GraphTheme_t::CUSTOM) {
            sb.append_comma();

            const auto& theme = m_graphUI->viewSettings()->m_theme;
            sb.append_key_value<"background_color">(theme.m_backgroundColor);
            sb.append_comma();
            sb.append_key_value<"grid_color">(theme.m_gridColor);
            sb.append_comma();
            sb.append_key_value<"min_max_color">(theme.m_minMaxColor);
            sb.append_comma();

            sb.append_key_value<"node_color">(theme.m_nodeColor);
            sb.append_comma();
            sb.append_key_value<"node_outline_color">(theme.m_nodeOutlineColor);
            sb.append_comma();
            sb.append_key_value<"selected_node_outline_color">(theme.m_selectedNodeOutlineColor);
            sb.append_comma();
            sb.append_key_value<"hovered_node_outline_color">(theme.m_hoveredNodeOutlineColor);
            sb.append_comma();
            sb.append_key_value<"hovered_and_selected_node_outline_color">(
                theme.m_hoveredAndSelectedNodeOutlineColor);
        }
    }
    sb.end_object();

    std::ofstream settingsFile{Constants::uiSettingsFile};
    if (settingsFile) {
        settingsFile << sb.view();
    } else {
        common::Logger::get().warning("Couldn't open {} for writing.", Constants::uiSettingsFile);
    }
#endif
}

void SettingsManager::loadSettings() {
#ifndef __EMSCRIPTEN__
    using namespace simdjson;

    if (!m_graphUI || !m_documentHandler) {
        common::Logger::get().warning(
            "SettingsManager not properly initialized, skipping loading settings.");
        return;
    }

    try {
        ondemand::parser parser;
        const auto json = padded_string::load(Constants::uiSettingsFile);
        auto doc = parser.iterate(json);

        const auto uiThemeIndex =
            static_cast<GraphUI::UITheme>(doc["ui_theme_index"].get_int64().value());
        if (uiThemeIndex >= GraphUI::UITheme::IMGUI_WHITE &&
            uiThemeIndex < GraphUI::UITheme::UITHEME_COUNT) {
            m_graphUI->currentTheme() = uiThemeIndex;
        }

        const auto graphThemeIndex = static_cast<int>(doc["graph_theme_index"].get_int64().value());
        if (graphThemeIndex == 0 || graphThemeIndex == 1 || graphThemeIndex == 2) {
            m_graphUI->currentGraphTheme() = static_cast<GraphUI::GraphTheme_t>(graphThemeIndex);
        }

        m_graphUI->isSettingsOpen() = doc["settings_open"].get_bool().value();
        m_graphUI->isFullScreen() = doc["fullscreen"].get_bool().value();
        m_graphUI->fileViewOpen() = doc["file_view_open"].get_bool().value();
        m_graphUI->inspectorOpen() = doc["inspector_open"].get_bool().value();
        m_graphUI->nodeViewerOpen() = doc["node_viewer_open"].get_bool().value();
        m_graphUI->logsOpen() = doc["logs_open"].get_bool().value();
        m_graphUI->getVsyncMode() = static_cast<int>(doc["vsync_mode"].get_int64().value());
        m_graphUI->isFpsLimitEnabled() = doc["fps_limit_enabled"].get_bool().value();
        m_graphUI->getMaxFps() = static_cast<int>(doc["max_fps"].get_int64().value());

        const auto openedRootFolder = doc["opened_root_folder"].get_string().value();
        m_graphUI->setOpenedRootFolder(std::string(openedRootFolder));

        const auto openedDocumentIndex =
            static_cast<size_t>(doc["opened_document_index"].get_int64().value());

        bool loadedAtLeastOneDocument = false;
        for (auto graphPath : doc["graphs_paths"]) {
            const auto pathStr = std::string(graphPath.get_string().value());
            m_documentHandler->scheduleOpenDocument(pathStr);
            loadedAtLeastOneDocument = true;
        }

        if (loadedAtLeastOneDocument) {
            m_documentHandler->scheduleCloseDocument(0);
            m_documentHandler->scheduleOpenedDocument(openedDocumentIndex);
        }

        if (graphThemeIndex == 2) {
            auto& theme = m_graphUI->viewSettings()->m_theme;
            theme.m_backgroundColor = (ImU32)doc["background_color"].get_uint64().value();
            theme.m_gridColor = (ImU32)doc["grid_color"].get_uint64().value();
            theme.m_minMaxColor = (ImU32)doc["min_max_color"].get_uint64().value();
            theme.m_nodeColor = (ImU32)doc["node_color"].get_uint64().value();
            theme.m_nodeOutlineColor = (ImU32)doc["node_outline_color"].get_uint64().value();
            theme.m_selectedNodeOutlineColor =
                (ImU32)doc["selected_node_outline_color"].get_uint64().value();
            theme.m_hoveredNodeOutlineColor =
                (ImU32)doc["hovered_node_outline_color"].get_uint64().value();
            theme.m_hoveredAndSelectedNodeOutlineColor =
                (ImU32)doc["hovered_and_selected_node_outline_color"].get_uint64().value();
            m_graphUI->viewSettings()->m_shouldFullColorNodes = true;
        }
    } catch (const std::exception& e) {
        common::Logger::get().warning("Failed to load UI settings from JSON: {}", e.what());
    }
#endif
}
