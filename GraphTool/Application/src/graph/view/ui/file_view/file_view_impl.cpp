module;
#include <pch.h>

module file_view;

import graph_model_defines;

void FileView::initialize(GraphDocumentHandler* docHandler) {
    m_documentHandler = docHandler;
    refreshRootFolder();
}

void FileView::render() {
    if (!m_isOpen) {
        return;
    }

    if (ImGui::Begin("File View", &m_isOpen)) {
#ifndef __EMSCRIPTEN__
        if (ImGui::Button("Change Root Folder", {-FLT_MIN, 0})) {
            openFolderDialog();
        }
#endif

        ImGui::BeginChild("FilesContainer", ImVec2(0, 0), true);

        if (ImGui::TreeNode(m_openedRootFolder.c_str())) {
            drawFileViewHelper(m_openedRootFolder, m_filesInRootFolder);
            ImGui::TreePop();
        }

        ImGui::EndChild();
    }
    ImGui::End();

    renderOSMPopup();
}

bool& FileView::isOpen() { return m_isOpen; }

void FileView::refreshRootFolder() {
    m_filesInRootFolder.clear();
    refreshFilesInFolder(m_openedRootFolder, m_filesInRootFolder);
}

void FileView::setOpenedRootFolder(const std::string& folder) { m_openedRootFolder = folder; }

const std::string& FileView::getOpenedRootFolder() const { return m_openedRootFolder; }

void FileView::openFolderDialog() {
#ifndef __EMSCRIPTEN__
    const auto result = tinyfd_selectFolderDialog("Select Root Folder", m_openedRootFolder.c_str());
    if (result) {
        m_openedRootFolder = result;
        refreshRootFolder();
    }
#endif
}

void FileView::renderOSMPopup() {
    if (!m_isOpenOSMPopup) {
        return;
    }

    ImGui::OpenPopup("Open OSM File");

    const auto& filePath = m_selectedFileEntry->m_path;
    auto& loadSettings = m_documentHandler->getOSMLoadSettings();

    const auto centerPos = ImGui::GetIO().DisplaySize * 0.5f;
    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Open OSM File", &m_isOpenOSMPopup)) {
        if (ImGui::BeginTable("OSMLoadSettingsTable", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 180.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("World extent");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Defines the size of the world space used to project the map."
                    "\nThe map will be centered on the center of the loaded data, and the world"
                    "\nwill extend by this amount in all directions. The bigger the world extent,"
                    "\nthe better the precision when working with large maps, but it also "
                    "\nincreases memory usage.");
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputFloat("##wbs", &loadSettings.m_worldBounds, 500.f, 5000.f, "%.2f")) {
                loadSettings.m_worldBounds =
                    std::clamp(loadSettings.m_worldBounds, 500.f, WORLD_BOUNDS_FIXED_SIZE);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Merge nearby nodes");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "When enabled, nodes that are within the specified distance will be merged\n"
                    "into a single node. This can help reduce the number of nodes in the graph\n"
                    "and improve performance, but may result in a loss of detail.");
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##mnn", &loadSettings.m_mergeCloseNodes);

            ImGui::BeginDisabled(!loadSettings.m_mergeCloseNodes);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Merge distance");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "The distance threshold for merging nearby nodes.\nNodes that are closer than "
                    "this distance will be merged into a single node.");
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputFloat("##md", &loadSettings.m_mergeCloseNodesDistance, 0.5f, 10.f,
                                  "%.2f")) {
                loadSettings.m_mergeCloseNodesDistance =
                    std::clamp(loadSettings.m_mergeCloseNodesDistance, 0.5f, 100.f);
            }
            ImGui::EndDisabled();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Parse highways");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##phw", &loadSettings.m_shouldParseHighways);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Parse railways");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##prw", &loadSettings.m_shouldParseRailways);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Parse boundaries");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##pb", &loadSettings.m_shouldParseBoundaries);

            ImGui::EndTable();
        }

        const auto shouldParseSomething = loadSettings.m_shouldParseHighways ||
                                          loadSettings.m_shouldParseBoundaries ||
                                          loadSettings.m_shouldParseRailways;

        if (loadSettings.m_shouldParseHighways || loadSettings.m_shouldParseRailways) {
            enum class ParseItemSelectedTab {
                HIGHWAYS,
                RAILWAYS
            } static selectedTab = ParseItemSelectedTab::HIGHWAYS;

            if (ImGui::BeginTabBar("ParseItem")) {
                if (loadSettings.m_shouldParseHighways && ImGui::BeginTabItem("Highways")) {
                    selectedTab = ParseItemSelectedTab::HIGHWAYS;
                    ImGui::EndTabItem();
                }

                if (loadSettings.m_shouldParseRailways && ImGui::BeginTabItem("Railways")) {
                    selectedTab = ParseItemSelectedTab::RAILWAYS;
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::BeginChild("ParseSettingsChild", ImVec2(0, 256), true);
            if (ImGui::BeginTable("ParseSettingsTable", 2, ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 100.0f);

                const auto checkBox = [this](const char* label, bool& value) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    drawTextCentered(label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushID(&value);
                    ImGui::Checkbox("##pscb", &value);
                    ImGui::PopID();
                };

                switch (selectedTab) {
                    case ParseItemSelectedTab::HIGHWAYS: {
                        checkBox("Parse motorways", loadSettings.m_parseMotorways);
                        checkBox("Parse motorway links", loadSettings.m_parseMotorwayLinks);

                        checkBox("Parse trunks", loadSettings.m_parseTrunks);
                        checkBox("Parse trunk links", loadSettings.m_parseTrunkLinks);

                        checkBox("Parse primary", loadSettings.m_parsePrimarys);
                        checkBox("Parse primary links", loadSettings.m_parsePrimaryLinks);

                        checkBox("Parse secondary", loadSettings.m_parseSecondarys);
                        checkBox("Parse secondary links", loadSettings.m_parseSecondaryLinks);

                        checkBox("Parse tertiary", loadSettings.m_parseTertiarys);
                        checkBox("Parse tertiary links", loadSettings.m_parseTertiaryLinks);

                        checkBox("Parse unclassified", loadSettings.m_parseUnclassifieds);
                        checkBox("Parse residential", loadSettings.m_parseResidentials);
                        checkBox("Parse living streets", loadSettings.m_parseLivingStreets);
                        checkBox("Parse services ways", loadSettings.m_parseServices);
                        checkBox("Parse pedestrians ways", loadSettings.m_parsePedestrians);

                        break;
                    }

                    case ParseItemSelectedTab::RAILWAYS: {
                        checkBox("Parse rails", loadSettings.m_parseRails);
                        checkBox("Parse light rails", loadSettings.m_parseLightRails);
                        checkBox("Parse subways", loadSettings.m_parseSubways);
                        checkBox("Parse trams", loadSettings.m_parseTrams);

                        break;
                    }
                }

                ImGui::EndTable();
            }
            ImGui::EndChild();
        }

        const auto availableWidth = ImGui::GetContentRegionAvail().x;
        const auto itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        const auto buttonWidth = (availableWidth - itemSpacing) * 0.5f;

        ImGui::Separator();
        ImGui::BeginDisabled(!shouldParseSomething);
        if (ImGui::Button("Open", {buttonWidth, 0})) {
            m_documentHandler->scheduleOpenDocument(filePath);
            m_isOpenOSMPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", {buttonWidth, 0}) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_isOpenOSMPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void FileView::drawTextCentered(const char* fmt, ...) {
    float textHeight = ImGui::GetTextLineHeight();
    float sliderHeight = ImGui::GetFrameHeight();
    float offsetY = (sliderHeight - textHeight) * 0.5f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

    char text[256];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    ImGui::TextWrapped("%s", text);
}

void FileView::refreshFilesInFolder(const std::string& folder, std::vector<FileEntry>& fileEntry) {
    std::filesystem::path rootPath{folder};
    for (const auto& entry : std::filesystem::directory_iterator(rootPath)) {
        const auto& path = entry.path();
        if (entry.is_regular_file()) {
            if (path.extension() == ".bin") {
                fileEntry.emplace_back(path.string(), FileEntry::Type::FILE);
            }
#ifndef __EMSCRIPTEN__
            else if (path.extension() == ".osm" || path.extension() == ".pbf") {
                fileEntry.emplace_back(path.string(), FileEntry::Type::PBF_FILE);
            }
#endif
        } else if (entry.is_directory()) {
            fileEntry.emplace_back(path.string(), FileEntry::Type::FOLDER);
        }
    }
}

void FileView::drawFileViewHelper(const std::string& folder, std::vector<FileEntry>& fileEntry) {
    for (auto& entry : fileEntry) {
        if (entry.m_type == FileEntry::Type::FOLDER) {
            if (ImGui::TreeNode(entry.getName())) {
                if (!entry.m_isExpanded) {
                    refreshFilesInFolder(entry.m_path, entry.m_children);
                    entry.m_isExpanded = true;
                }

                drawFileViewHelper(entry.m_path, entry.m_children);
                ImGui::TreePop();
            } else {
                entry.m_isExpanded = false;
                entry.m_children.clear();
            }
        } else {
            if (ImGui::Selectable(entry.getName(), m_selectedFileEntry == &entry)) {
                m_selectedFileEntry = &entry;

                if (entry.m_type == FileEntry::Type::PBF_FILE) {
                    m_isOpenOSMPopup = true;
                } else {
                    m_documentHandler->scheduleOpenDocument(entry.m_path);
                }
            }
        }
    }
}
