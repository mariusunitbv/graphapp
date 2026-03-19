module;
#include <pch.h>

module graph_ui;

import texture_loader;

GraphUI::~GraphUI() {
    if (m_unitbvLogoTexture) {
        TextureLoader::unloadTexture(m_unitbvLogoTexture);
    }
}

void GraphUI::initialize(GraphViewSettings* viewSettings, GraphDocumentHandler* docHandler) {
    m_viewSettings = viewSettings;
    m_documentHandler = docHandler;
    m_defaultStyle = ImGui::GetStyle();

    common::Logger::get().addListener(this);

    initializeTextures();
    refreshRootFolder();
}

void GraphUI::preRenderUpdate(const GraphModel* model, GraphViewModel* viewModel) {
    m_model = model;
    m_viewModel = viewModel;

    static UITheme lastTheme = m_currentTheme;
    if (m_currentTheme != lastTheme) {
        onThemeSwitched();
        lastTheme = m_currentTheme;
    }

    static int lastGraphTheme = m_currentGraphTheme;
    if (m_currentGraphTheme != lastGraphTheme) {
        onGraphThemeSwitched();
        lastGraphTheme = m_currentGraphTheme;
    }
}

void GraphUI::onSDLEvent(const SDL_Event& event) {
    if (isFocusOnUI()) {
        return;
    }

    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            switch (event.key.key) {
                case SDLK_DELETE:
                    if (m_viewModel->getSelectedNodesCount() > 0) {
                        m_isDeleteDialogOpen = true;
                    }

                    break;
                case SDLK_C:
                    m_isCenterOnNodeDialogOpen = true;
                    break;
                case SDLK_G:
                    m_viewSettings->m_drawGrid = !m_viewSettings->m_drawGrid;
                    break;
                case SDLK_N:
                    m_viewSettings->m_drawNodes = !m_viewSettings->m_drawNodes;
                    break;
                case SDLK_E:
                    m_viewSettings->m_drawEdges = !m_viewSettings->m_drawEdges;
                    break;
                case SDLK_F12:
                    m_isSettingsOpen = !m_isSettingsOpen;
                    break;
            }

            break;
    }
}

void GraphUI::refreshRootFolder() {
    m_filesInRootFolder.clear();
    refreshFilesInFolder(m_openedRootFolder, m_filesInRootFolder);
}

void GraphUI::render(const std::vector<GraphDocument>& openDocuments,
                     size_t& currentOpenedDocument) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    if (!isFocusOnUI() && m_viewModel->getHoveredNodeIndex() != INVALID_NODE) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    drawMenuBar();
    drawDeleteConfirmationDialog();
    drawCenterOnNodeDialog();

    setupDockSpace();

    drawFileView();
    drawInspector();
    drawOpenedTabs(openDocuments, currentOpenedDocument);
    drawLogsWindow();
    drawStatusBar();
    drawSettings();

    // We don't need focus the first time the window appears.
    static bool initialized = false;
    if (!initialized) {
        ImGui::SetWindowFocus(nullptr);
        initialized = true;
    }

    drawUnfocusedBackground(drawList);
    drawAddNodesText(drawList);
    drawVersion(drawList);
    drawWatermark(ImGui::GetForegroundDrawList());
}

bool GraphUI::isFocusOnUI() const {
    return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

int GraphUI::getVsyncMode() const {
    // https://wiki.libsdl.org/SDL3/SDL_GL_SetSwapInterval
    if (m_vsyncMode == 2) {
        return -1;
    }

    return m_vsyncMode;
}

void GraphUI::onLogMessage(common::Logger::Level level, const std::string_view message) {
    std::unique_lock lock(m_logMutex);

    auto lineStart = m_logBuffer.size();
    m_logBuffer.insert(m_logBuffer.end(), message.begin(), message.end());
    m_logBuffer.push_back('\n');

    for (size_t i = 0; i < message.size(); ++i) {
        if (message[i] == '\n') {
            m_logLines.emplace_back(static_cast<uint32_t>(lineStart), static_cast<uint32_t>(level));
            lineStart = m_logBuffer.size() - message.size() + i + 1;
        }
    }
}

void GraphUI::initializeTextures() {
    m_unitbvLogoTexture = TextureLoader::loadPNGFile("assets/unitbv.png");
}

void GraphUI::setupDockSpace() {
    ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (!ImGui::DockBuilderGetNode(dockspaceId)) {
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

        ImGuiID mainDockID = dockspaceId;
        ImGuiID fileViewID{}, inspectorViewID{}, tabViewID{}, logsViewID{};
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Left, 0.2f, &fileViewID, &mainDockID);
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Right, 0.45f, &inspectorViewID,
                                    &mainDockID);
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Up, 0.06f, &tabViewID, nullptr);
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Down, 0.35f, &logsViewID, nullptr);

        ImGui::DockBuilderDockWindow("File View", fileViewID);
        ImGui::DockBuilderDockWindow("Inspector", inspectorViewID);
        ImGui::DockBuilderDockWindow("Tab Area", tabViewID);
        ImGui::DockBuilderDockWindow("Logs", logsViewID);

        ImGui::DockBuilderFinish(dockspaceId);

        ImGuiDockNode* tabNode = ImGui::DockBuilderGetNode(tabViewID);
        if (tabNode) {
            tabNode->LocalFlags |= ImGuiDockNodeFlags_NoUndocking;
            tabNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
            tabNode->LocalFlags |= ImGuiDockNodeFlags_NoDocking;
            tabNode->LocalFlags |= ImGuiDockNodeFlags_NoResizeY;
        }
    }

    const float statusBarHeight = ImGui::GetFont()->FontSize + 3.f;

    ImGui::SetNextWindowPos(viewport->WorkPos - ImVec2{1, 1});
    ImGui::SetNextWindowSize(viewport->WorkSize - ImVec2{-2, statusBarHeight - 2});
    ImGui::SetNextWindowViewport(viewport->ID);

    constexpr auto window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoDocking |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus |
                                  ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("DockSpace Window", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::DockSpace(dockspaceId, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    const auto centralNode = ImGui::DockBuilderGetCentralNode(dockspaceId);
    if (centralNode) {
        m_sceneViewPos = centralNode->Pos;
        m_sceneViewSize = centralNode->Size;
    }
}

void GraphUI::drawMenuBar() {
    ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, 12);
    if (ImGui::BeginMainMenuBar()) {
        ImGui::PopStyleVar();
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Center on Node", "C")) {
                m_isCenterOnNodeDialogOpen = true;
            }

            if (ImGui::MenuItem("Refresh Graph", "F5")) {
                m_viewModel->refreshVisibleData();
            }

            ImGui::MenuItem("Render Grid", "G", &m_viewSettings->m_drawGrid);
            ImGui::MenuItem("Highlight Extents", nullptr, &m_viewSettings->m_drawMinMax);

            if (ImGui::BeginMenu("Graph Elements")) {
                ImGui::MenuItem("Show Nodes", "N", &m_viewSettings->m_drawNodes);
                ImGui::MenuItem("Show Nodes Outline", nullptr, &m_viewSettings->m_drawNodesOutline);
                ImGui::MenuItem("Show Edges", "E", &m_viewSettings->m_drawEdges);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("UI Elements")) {
                ImGui::MenuItem("Show File View", nullptr, &m_fileViewOpen);
                ImGui::MenuItem("Show Inspector", nullptr, &m_inspectorOpen);
                ImGui::MenuItem("Show Logs", nullptr, &m_logsWindowOpen);
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Selection", m_viewModel->getSelectedNodesCount() > 0)) {
            ImGui::Text("Selected count: %zu", m_viewModel->getSelectedNodesCount());
            ImGui::Separator();

            if (ImGui::MenuItem("Remove", "Delete")) {
                m_isDeleteDialogOpen = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings")) {
            if (ImGui::MenuItem("Options", "F12")) {
                m_isSettingsOpen = true;
            }

            if (ImGui::BeginMenu("UI Theme")) {
                for (int i = 0; i < static_cast<int>(UITheme::UITHEME_COUNT); ++i) {
                    const auto theme = static_cast<UITheme>(i);
                    const bool selected = (theme == m_currentTheme);
                    if (ImGui::MenuItem(m_themeNames[i].data(), nullptr, selected)) {
                        m_currentTheme = theme;
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Graph Theme")) {
                if (ImGui::MenuItem("Dark", nullptr, m_currentGraphTheme == 0)) {
                    m_currentGraphTheme = 0;
                }

                if (ImGui::MenuItem("Light", nullptr, m_currentGraphTheme == 1)) {
                    m_currentGraphTheme = 1;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        const auto fps = ImGui::GetIO().Framerate;

        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "FPS: %.1f%s | %.2f ms", fps,
                      m_isFpsLimitEnabled ? "L" : "", 1000.f / fps);

        const auto textWidth = ImGui::CalcTextSize(buffer).x;
        if (textWidth * 1.2f < ImGui::GetContentRegionAvail().x) {
            const auto windowWidth = ImGui::GetWindowWidth();
            ImGui::SetCursorPosX(windowWidth - textWidth - 10.0f);
            ImGui::Text("%s", buffer);
        }

        ImGui::EndMainMenuBar();
    }
}

void GraphUI::drawOpenedTabs(const std::vector<GraphDocument>& openDocuments,
                             size_t& currentOpenedDocument) {
    constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("Tab Area", nullptr, flags)) {
        if (ImGui::BeginTabBar("docstab", ImGuiTabBarFlags_Reorderable |
                                              ImGuiTabBarFlags_DrawSelectedOverline |
                                              ImGuiTabBarFlags_FittingPolicyScroll)) {
            for (size_t i = 0; i < openDocuments.size(); ++i) {
                const auto& doc = openDocuments[i];

                ImGui::PushID(doc.m_path.c_str());

                bool isOpen = true;
                if (ImGui::BeginTabItem(doc.getName(), &isOpen)) {
                    currentOpenedDocument = i;
                    ImGui::EndTabItem();
                }

                if (!isOpen) {
                    m_documentHandler->scheduleCloseDocument(i);
                }

                ImGui::PopID();
            }
            ImGui::EndTabBar();
        }
    }

    ImGui::End();
}

void GraphUI::drawLogsWindow() {
    if (!m_logsWindowOpen) {
        return;
    }

    const auto& style = ImGui::GetStyle();
    const ImVec4 logLUT[] = {style.Colors[ImGuiCol_TextDisabled],  // DEBUG_LEVEL
                             style.Colors[ImGuiCol_Text],          // INFORMATION_LEVEL
                             ImVec4(0.85f, 0.85f, 0.4f, 1.f),      // WARNING_LEVEL
                             ImVec4(1.f, 0.5f, 0.5f, 1.f),         // ERROR_LEVEL
                             ImVec4(1.f, 1.f, 1.f, 1.f)};

    if (ImGui::Begin("Logs", &m_logsWindowOpen)) {
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

void GraphUI::drawStatusBar() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));

    const auto barHeight = ImGui::GetFont()->FontSize + 3.f;
    const auto [screenWidth, screenHeight] = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, screenHeight - barHeight));
    ImGui::SetNextWindowSize(ImVec2(screenWidth, barHeight));
    ImGui::Begin("StatusBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);

    ImGui::Text("Zoom: %d%% %s", (int)std::lround(m_viewModel->getZoomFactor() * 100.f),
                isFocusOnUI() ? "(UNFOCUSED)" : "");

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void GraphUI::drawDeleteConfirmationDialog() {
    if (!m_isDeleteDialogOpen) {
        return;
    }

    ImGui::OpenPopup("Confirmation");

    const auto centerPos = ImGui::GetIO().DisplaySize * 0.5f;
    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Confirmation", &m_isDeleteDialogOpen,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        const auto nodeCount = m_viewModel->getSelectedNodesCount();
        ImGui::Text("Are you sure you want to delete %zu node%s?", nodeCount,
                    nodeCount == 1 ? "" : "s");
        ImGui::Separator();

        const auto availableWidth = ImGui::GetContentRegionAvail().x;
        const auto itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        const auto buttonWidth = (availableWidth - itemSpacing) * 0.5f;

        if (ImGui::Button("Yes", ImVec2(buttonWidth, 0))) {
            m_viewModel->removeSelectedNodes();
            m_isDeleteDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetNavCursorVisible(true);
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        if (ImGui::Button("No", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_isDeleteDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void GraphUI::drawCenterOnNodeDialog() {
    if (!m_isCenterOnNodeDialogOpen) {
        return;
    }

    ImGui::OpenPopup("Enter Node");

    static int nodeId = 0;

    const auto centerPos = ImGui::GetIO().DisplaySize * 0.5f + ImVec2{0, 100};
    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Enter Node", &m_isCenterOnNodeDialogOpen,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        const auto escapePressed = ImGui::IsKeyPressed(ImGuiKey_Escape);

        ImGui::TextUnformatted("Enter Node to center on:");
        if (ImGui::InputInt("##nodeid", &nodeId, 0, 1000) && !escapePressed) {
            nodeId = std::clamp(nodeId, 0, static_cast<int>(m_model->getLastNodeIndex()));
            m_viewModel->centerOnNode(nodeId);
        }

        if (ImGui::IsWindowAppearing()) {
            ImGui::ActivateItemByID(ImGui::GetItemID());
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            nodeId = std::clamp(nodeId, 0, static_cast<int>(m_model->getLastNodeIndex()));
            m_viewModel->centerOnNode(nodeId);
            m_isCenterOnNodeDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }

        if (escapePressed) {
            m_isCenterOnNodeDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void GraphUI::drawFileView() {
    if (!m_fileViewOpen) {
        return;
    }

    if (ImGui::Begin("File View", &m_fileViewOpen)) {
        ImGui::TextWrapped("Root Folder: %s", m_openedRootFolder.c_str());
        ImGui::BeginChild("FilesContainer", ImVec2(0, 0), true);
        drawFileViewHelper(m_openedRootFolder, m_filesInRootFolder);
        ImGui::EndChild();
    }
    ImGui::End();
}

void GraphUI::drawInspector() {
    if (!m_inspectorOpen) {
        return;
    }

    if (ImGui::Begin("Inspector", &m_inspectorOpen)) {
        ImGui::SeparatorText("Graph Details");

        if (ImGui::BeginTable("InspectorTable", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Slider", ImGuiTableColumnFlags_WidthFixed, 140.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Zoom factor");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            auto graphZoom = m_viewModel->getZoomFactor();
            if (ImGui::SliderFloat("##graphZoom", &graphZoom, 0.05f, 50.f, "%.2fx")) {
                graphZoom = std::clamp(graphZoom, 0.05f, 50.f);
                m_viewModel->setZoomFactor(graphZoom);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Node radius");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            auto nodeRadius = m_viewModel->getNodesRadius();
            if (ImGui::SliderFloat("##nrs", &nodeRadius, 0.5f, 100.f, "%.2fpx")) {
                nodeRadius = std::clamp(nodeRadius, 0.5f, 100.f);
                m_viewModel->setNodesRadius(nodeRadius);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Shown node limit");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            int maxNodes = m_viewModel->getMaxVisibleNodes();
            if (ImGui::SliderInt("##maxNodes", &maxNodes, 10'000, 50'000'000)) {
                maxNodes = std::clamp(maxNodes, 10'000, 50'000'000);
                m_viewModel->setMaxVisibleNodes(maxNodes);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Skip nodes on low zoom");

            ImGui::TableSetColumnIndex(1);
            bool shouldCondensateNodes = m_viewModel->shouldCondensateNodesLowZoom();
            if (ImGui::Checkbox("##cdst", &shouldCondensateNodes)) {
                m_viewModel->setShouldCondensateNodesLowZoom(shouldCondensateNodes);
            }

            ImGui::BeginDisabled(!shouldCondensateNodes);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Zoom limit for skipping");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Determines at which zoom factor the nodes start to skip.\nA lower "
                    "value "
                    "means that nodes will start being skipped at a lower zoom level.");
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            auto nodeCondensationFactor = m_viewModel->getNodeCondensationPercentage();
            if (ImGui::SliderFloat("##ndcf", &nodeCondensationFactor, 0.05f, 0.7f, "%.2fx")) {
                nodeCondensationFactor = std::clamp(nodeCondensationFactor, 0.05f, 0.7f);
                m_viewModel->setNodeCondensationPercentage(nodeCondensationFactor);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("%% of nodes per cell");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "The world is divided into cells of size of %dpx each.\n"
                    "This setting determines how many nodes will be drawn in\neach cell "
                    "when the zoom level is low enough for skipping to occur.\nA lower "
                    "value "
                    "means that fewer "
                    "nodes will be drawn in each cell, which\ncan improve performance but may "
                    "make "
                    "the graph look more sparse.",
                    m_model->getGridMapCellSize());
            }

            auto maxNodesPerCellBase = m_viewModel->getMaxNodesPerCellBase();
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderInt("##ndpcb", &maxNodesPerCellBase, 1, 100, "%d%%")) {
                maxNodesPerCellBase = std::clamp(maxNodesPerCellBase, 1, 100);
                m_viewModel->setMaxNodesPerCellBase(maxNodesPerCellBase);
            }

            ImGui::EndDisabled();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("%% of edges per node");

            ImGui::TableSetColumnIndex(1);
            int edgeDrawPercentage = m_viewModel->getEdgeDrawPercentage();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderInt("##edp", &edgeDrawPercentage, 1, 100, "%d%%")) {
                edgeDrawPercentage = std::clamp(edgeDrawPercentage, 1, 100);
                m_viewModel->setEdgeDrawPercentage(edgeDrawPercentage);
            }

            ImGui::EndTable();
        }

        ImGui::SeparatorText("Graph statistics");

        if (ImGui::BeginTable(
                "StatsTable", 2,
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Total nodes");

            ImGui::TableSetColumnIndex(1);

            const auto lastNodeIndex = m_model->getLastNodeIndex();
            drawTextCentered("%zu", lastNodeIndex == INVALID_NODE ? 0u : lastNodeIndex + 1);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Visible nodes");

            ImGui::TableSetColumnIndex(1);

            drawTextCentered("%llu", m_viewModel->getVisibleNodes().size());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Visible edges");

            ImGui::TableSetColumnIndex(1);

            drawTextCentered("%llu", m_viewModel->getVisibleEdges().size());

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void GraphUI::drawSettings() {
    if (!m_isSettingsOpen) {
        return;
    }

    const auto& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(550, 520), ImGuiCond_Once);

    constexpr const char* settingsTabs[] = {"Appearance", "Performance", "Display"};
    static int currentTab = 0;

    ImGui::Begin("Settings", &m_isSettingsOpen);

    if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None)) {
        for (int i = 0; i < std::size(settingsTabs); ++i) {
            if (ImGui::BeginTabItem(settingsTabs[i])) {
                currentTab = i;
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::BeginChild("##settings_content", ImVec2(0, 0), ImGuiChildFlags_Borders);
    if (currentTab == 0) {
        ImGui::SeparatorText("UI Appearance");

        if (ImGui::BeginTable("UIAppearanceSettings", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("UI Theme");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##uitheme",
                                  m_themeNames[static_cast<int>(m_currentTheme)].data())) {
                for (int i = 0; i < static_cast<int>(UITheme::UITHEME_COUNT); ++i) {
                    const auto theme = static_cast<UITheme>(i);
                    const bool selected = (theme == m_currentTheme);
                    if (ImGui::Selectable(m_themeNames[i].data(), selected)) {
                        m_currentTheme = theme;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Graph Theme");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##graphtheme", m_currentGraphTheme == 0 ? "Dark" : "Light")) {
                if (ImGui::Selectable("Dark", m_currentGraphTheme == 0)) {
                    m_currentGraphTheme = 0;
                }

                if (ImGui::Selectable("Light", m_currentGraphTheme == 1)) {
                    m_currentGraphTheme = 1;
                }

                ImGui::EndCombo();
            }

            ImGui::EndTable();
        }

        ImGui::SeparatorText("Graph Appearance");

        const auto drawColorPicker = [this](const char* label, uint32_t& color) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered(label);

            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(label);

            auto colorVec = ImGui::ColorConvertU32ToFloat4(color);
            if (ImGui::ColorEdit4("##picker", (float*)&colorVec, ImGuiColorEditFlags_NoInputs)) {
                color = ImGui::ColorConvertFloat4ToU32(colorVec);
            }
            ImGui::PopID();
        };

        auto& theme = m_viewSettings->m_theme;
        if (ImGui::BeginTable("GraphAppearanceSettings", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

            drawColorPicker("Graph background", theme.m_backgroundColor);
            drawColorPicker("Grid lines", theme.m_gridColor);
            drawColorPicker("Graph extent", theme.m_minMaxColor);

            ImGui::EndTable();
        }

        ImGui::SeparatorText("Node Appearance");

        if (ImGui::BeginTable("NodeAppearanceSettings", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

            drawColorPicker("Default color", theme.m_nodeColor);
            drawColorPicker("Default outline", theme.m_nodeOutlineColor);
            drawColorPicker("Selected outline", theme.m_selectedNodeOutlineColor);
            drawColorPicker("Hovered outline", theme.m_hoveredNodeOutlineColor);
            drawColorPicker("Selected and hovered outline",
                            theme.m_hoveredAndSelectedNodeOutlineColor);

            ImGui::EndTable();
        }

        if (ImGui::Button("Force Full Update", {-FLT_MIN, 0})) {
            m_viewSettings->m_shouldFullColorNodes = true;
        }
    } else if (currentTab == 1) {
        ImGui::SeparatorText("Performance");

        if (ImGui::BeginTable("PerformanceSettings", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

#ifndef __EMSCRIPTEN__
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Fullscreen mode");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##fullscreen", &m_appFullScreen);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Limit FPS");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##fpsLimitEnabled", &m_isFpsLimitEnabled);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Maximum FPS");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderInt("##fpsLimit", &m_maxFps, 5, 360)) {
                m_maxFps = std::clamp(m_maxFps, 5, 360);
            }
#endif

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("VSync mode");

            ImGui::TableSetColumnIndex(1);

            constexpr const char* vsyncOptions[] = {"Off", "On", "Adaptive"};
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##vsync", vsyncOptions[m_vsyncMode])) {
                for (int i = 0; i < std::size(vsyncOptions); ++i) {
                    if (ImGui::Selectable(vsyncOptions[i], m_vsyncMode == i)) {
                        m_vsyncMode = i;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Minimum zoom to show nodes");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderInt("##nodeZoom", &m_viewSettings->m_nodeCutoffZoom, 0, 500, "%d%%")) {
                m_viewSettings->m_nodeCutoffZoom =
                    std::clamp(m_viewSettings->m_nodeCutoffZoom, 0, 500);
            }

            ImGui::EndTable();
        }
    } else if (currentTab == 2) {
        ImGui::SeparatorText("Display");

        if (ImGui::BeginTable("DisplaySettings", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Draw grid");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##drawGrid", &m_viewSettings->m_drawGrid);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Draw graph extents");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##drawMinMax", &m_viewSettings->m_drawMinMax);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Draw nodes");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##drawNodes", &m_viewSettings->m_drawNodes);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Draw nodes outline");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##drawNodesOutline", &m_viewSettings->m_drawNodesOutline);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Draw edges");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##drawEdges", &m_viewSettings->m_drawEdges);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Node outline thickness");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderInt("##outlineThickness", &m_viewSettings->m_outlineThickness, 1, 4)) {
                m_viewSettings->m_outlineThickness =
                    std::clamp(m_viewSettings->m_outlineThickness, 1, 4);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Graph font size");

            ImGui::TableSetColumnIndex(1);

            static constexpr const char* fontLabels[] = {"Bigger", "Smaller"};
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##fsize", fontLabels[m_viewSettings->m_graphTextFontIndex])) {
                for (int i = 0; i < std::size(fontLabels); ++i) {
                    if (ImGui::Selectable(fontLabels[i],
                                          m_viewSettings->m_graphTextFontIndex == i)) {
                        m_viewSettings->m_graphTextFontIndex = i;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Grid spacing");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##gridSpacing", &m_viewSettings->m_gridCellSize, 10.f, 100.f,
                                   "%.2fpx")) {
                m_viewSettings->m_gridCellSize =
                    std::clamp(m_viewSettings->m_gridCellSize, 10.f, 100.f);
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void GraphUI::drawUnfocusedBackground(ImDrawList* drawList) {
    if (!isFocusOnUI()) {
        return;
    }

    const auto displaySize = ImGui::GetIO().DisplaySize;

    drawList->AddRectFilled({0, 0}, displaySize, IM_COL32(0, 0, 0, 60));

    constexpr auto unfocusedText = "Unfocused. Left Click to focus.";
    const auto font = ImGui::GetIO().Fonts->Fonts[m_viewSettings->m_graphTextFontIndex];
    const auto textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, unfocusedText);
    const auto textPos = m_sceneViewPos + (m_sceneViewSize - textSize) * 0.5f;

    drawList->AddRectFilled(textPos - ImVec2(5, 5), textPos + textSize + ImVec2(5, 5),
                            IM_COL32(0, 0, 0, 140));
    drawList->AddText(font, font->FontSize, textPos, IM_COL32(255, 255, 255, 255), unfocusedText);
}

void GraphUI::drawAddNodesText(ImDrawList* drawList) {
    if (isFocusOnUI() || m_model->getLastNodeIndex() != INVALID_NODE) {
        return;
    }

    constexpr auto helperText = "Left Click to add nodes.";
    const auto font = ImGui::GetIO().Fonts->Fonts[m_viewSettings->m_graphTextFontIndex];
    const auto textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, helperText);
    const auto textPos = m_sceneViewPos + (m_sceneViewSize - textSize) * 0.5f;

    drawList->AddRectFilled(textPos - ImVec2(5, 5), textPos + textSize + ImVec2(5, 5),
                            IM_COL32(0, 0, 0, 140));
    drawList->AddText(font, font->FontSize, textPos, IM_COL32(255, 255, 255, 255), helperText);
}

void GraphUI::drawVersion(ImDrawList* drawList) {
    const auto font = ImGui::GetIO().Fonts->Fonts[1];

    const auto pos = m_sceneViewPos;
    const auto versionSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, GAPP_VERSION);

    drawList->AddRectFilled(pos + ImVec2{8, 8}, pos + ImVec2{16, 16} + versionSize,
                            IM_COL32(0, 0, 0, 120), 5.f);
    drawList->AddText(font, font->FontSize, pos + ImVec2{12, 12}, IM_COL32_WHITE, GAPP_VERSION);
}

void GraphUI::drawWatermark(ImDrawList* drawList) {
    const auto [width, height] = ImGui::GetIO().DisplaySize;

    const auto font = ImGui::GetIO().Fonts->Fonts[1];
    constexpr auto waterMarkText = "github.com/mariusunitbv/graphapp";
    const auto watermarkSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, waterMarkText);
    const auto textPos = ImVec2{28.f, height - 52.f};

    const auto imagePos = textPos - ImVec2{font->FontSize + 5.f, 0};

    drawList->AddRectFilled(imagePos - ImVec2{4, 4}, textPos + watermarkSize + ImVec2{4, 4},
                            IM_COL32(0, 0, 0, 120), 5.f);

    const auto t = static_cast<float>(ImGui::GetTime());
    ImU32 rainbowColor = IM_COL32((int)((sin(t * 2.0f + 0) * 0.5f + 0.5f) * 255),
                                  (int)((sin(t * 2.0f + 2) * 0.5f + 0.5f) * 255),
                                  (int)((sin(t * 2.0f + 4) * 0.5f + 0.5f) * 255), 255);

    drawList->AddImage(m_unitbvLogoTexture, imagePos,
                       imagePos + ImVec2{watermarkSize.y, watermarkSize.y}, {0.f, 0.f}, {1.f, 1.f},
                       rainbowColor);

    drawList->AddText(font, font->FontSize, textPos + ImVec2{-1, 0}, IM_COL32_BLACK, waterMarkText);
    drawList->AddText(font, font->FontSize, textPos + ImVec2{1, 0}, IM_COL32_BLACK, waterMarkText);
    drawList->AddText(font, font->FontSize, textPos + ImVec2{0, -1}, IM_COL32_BLACK, waterMarkText);
    drawList->AddText(font, font->FontSize, textPos + ImVec2{0, 1}, IM_COL32_BLACK, waterMarkText);
    drawList->AddText(font, font->FontSize, textPos, IM_COL32_WHITE, waterMarkText);
}

void GraphUI::drawTextCentered(const char* fmt, ...) {
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

void GraphUI::onThemeSwitched() {
    ImGui::GetStyle() = m_defaultStyle;

    switch (m_currentTheme) {
        case UITheme::IMGUI_WHITE:
            ImGui::StyleColorsLight();
            break;
        case UITheme::IMGUI_DARK:
            ImGui::StyleColorsDark();
            break;
        case UITheme::IMGUI_CLASSIC:
            ImGui::StyleColorsClassic();
            break;
        case UITheme::CORPORATE_GREY:
            themeCorporateGrey();
            break;
        case UITheme::CATPPUCCIN:
            themeCatppuccin();
            break;
        case UITheme::CHERRY:
            themeCherry();
            break;
        case UITheme::VGUI:
            themeVGUI();
            break;
        default:
            break;
    }
}

void GraphUI::themeCorporateGrey() {
    // Credit: https://github.com/ocornut/imgui/issues/707#issuecomment-468798935

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
    colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

    style.PopupRounding = 3;

    style.WindowPadding = ImVec2(4, 4);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 2);

    style.ScrollbarSize = 18;

    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 0.f;

    style.WindowRounding = 3;
    style.ChildRounding = 3;
    style.FrameRounding = 3;
    style.ScrollbarRounding = 2;
    style.GrabRounding = 3;

#ifdef IMGUI_HAS_DOCK
    style.TabBorderSize = 0.f;
    style.TabRounding = 3;

    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
#endif
}

void GraphUI::themeCatppuccin() {
    // Credit: https://github.com/ocornut/imgui/issues/707#issuecomment-3592676777

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Catppuccin Mocha Palette
    // --------------------------------------------------------
    const ImVec4 base = ImVec4(0.117f, 0.117f, 0.172f, 1.0f);      // #1e1e2e
    const ImVec4 mantle = ImVec4(0.109f, 0.109f, 0.156f, 1.0f);    // #181825
    const ImVec4 surface0 = ImVec4(0.200f, 0.207f, 0.286f, 1.0f);  // #313244
    const ImVec4 surface1 = ImVec4(0.247f, 0.254f, 0.337f, 1.0f);  // #3f4056
    const ImVec4 surface2 = ImVec4(0.290f, 0.301f, 0.388f, 1.0f);  // #4a4d63
    const ImVec4 overlay0 = ImVec4(0.396f, 0.403f, 0.486f, 1.0f);  // #65677c
    const ImVec4 overlay2 = ImVec4(0.576f, 0.584f, 0.654f, 1.0f);  // #9399b2
    const ImVec4 text = ImVec4(0.803f, 0.815f, 0.878f, 1.0f);      // #cdd6f4
    const ImVec4 subtext0 = ImVec4(0.639f, 0.658f, 0.764f, 1.0f);  // #a3a8c3
    const ImVec4 mauve = ImVec4(0.796f, 0.698f, 0.972f, 1.0f);     // #cba6f7
    const ImVec4 peach = ImVec4(0.980f, 0.709f, 0.572f, 1.0f);     // #fab387
    const ImVec4 yellow = ImVec4(0.980f, 0.913f, 0.596f, 1.0f);    // #f9e2af
    const ImVec4 green = ImVec4(0.650f, 0.890f, 0.631f, 1.0f);     // #a6e3a1
    const ImVec4 teal = ImVec4(0.580f, 0.886f, 0.819f, 1.0f);      // #94e2d5
    const ImVec4 sapphire = ImVec4(0.458f, 0.784f, 0.878f, 1.0f);  // #74c7ec
    const ImVec4 blue = ImVec4(0.533f, 0.698f, 0.976f, 1.0f);      // #89b4fa
    const ImVec4 lavender = ImVec4(0.709f, 0.764f, 0.980f, 1.0f);  // #b4befe

    // Main window and backgrounds
    colors[ImGuiCol_WindowBg] = base;
    colors[ImGuiCol_ChildBg] = base;
    colors[ImGuiCol_PopupBg] = surface0;
    colors[ImGuiCol_Border] = surface1;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = surface0;
    colors[ImGuiCol_FrameBgHovered] = surface1;
    colors[ImGuiCol_FrameBgActive] = surface2;
    colors[ImGuiCol_TitleBg] = mantle;
    colors[ImGuiCol_TitleBgActive] = surface0;
    colors[ImGuiCol_TitleBgCollapsed] = mantle;
    colors[ImGuiCol_MenuBarBg] = mantle;
    colors[ImGuiCol_ScrollbarBg] = surface0;
    colors[ImGuiCol_ScrollbarGrab] = surface2;
    colors[ImGuiCol_ScrollbarGrabHovered] = overlay0;
    colors[ImGuiCol_ScrollbarGrabActive] = overlay2;
    colors[ImGuiCol_CheckMark] = green;
    colors[ImGuiCol_SliderGrab] = sapphire;
    colors[ImGuiCol_SliderGrabActive] = blue;
    colors[ImGuiCol_Button] = surface0;
    colors[ImGuiCol_ButtonHovered] = surface1;
    colors[ImGuiCol_ButtonActive] = surface2;
    colors[ImGuiCol_Header] = surface0;
    colors[ImGuiCol_HeaderHovered] = surface1;
    colors[ImGuiCol_HeaderActive] = surface2;
    colors[ImGuiCol_Separator] = surface1;
    colors[ImGuiCol_SeparatorHovered] = mauve;
    colors[ImGuiCol_SeparatorActive] = mauve;
    colors[ImGuiCol_ResizeGrip] = surface2;
    colors[ImGuiCol_ResizeGripHovered] = mauve;
    colors[ImGuiCol_ResizeGripActive] = mauve;
    colors[ImGuiCol_Tab] = surface0;
    colors[ImGuiCol_TabHovered] = surface2;
    colors[ImGuiCol_TabSelected] = surface1;
    colors[ImGuiCol_TabDimmed] = surface0;
    colors[ImGuiCol_TabDimmedSelected] = surface1;
    colors[ImGuiCol_DockingPreview] = sapphire;
    colors[ImGuiCol_DockingEmptyBg] = base;
    colors[ImGuiCol_PlotLines] = blue;
    colors[ImGuiCol_PlotLinesHovered] = peach;
    colors[ImGuiCol_PlotHistogram] = teal;
    colors[ImGuiCol_PlotHistogramHovered] = green;
    colors[ImGuiCol_TableHeaderBg] = surface0;
    colors[ImGuiCol_TableBorderStrong] = surface1;
    colors[ImGuiCol_TableBorderLight] = surface0;
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = surface2;
    colors[ImGuiCol_DragDropTarget] = yellow;
    colors[ImGuiCol_NavCursor] = lavender;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = subtext0;

    // Rounded corners
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    // Padding and spacing
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(5.0f, 3.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.IndentSpacing = 21.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    // Borders
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
}

void GraphUI::themeCherry() {
    // Credit: https://github.com/ocornut/imgui/issues/707#issuecomment-430613104

    // cherry colors, 3 intensities
#define HI(v) ImVec4(0.502f, 0.075f, 0.256f, v)
#define MED(v) ImVec4(0.455f, 0.198f, 0.301f, v)
#define LOW(v) ImVec4(0.232f, 0.201f, 0.271f, v)
// backgrounds (@todo: complete with BG_MED, BG_LOW)
#define BG(v) ImVec4(0.200f, 0.220f, 0.270f, v)
// text
#define TEXT(v) ImVec4(0.860f, 0.930f, 0.890f, v)

    auto& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = TEXT(0.78f);
    style.Colors[ImGuiCol_TextDisabled] = TEXT(0.28f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = BG(0.58f);
    style.Colors[ImGuiCol_PopupBg] = BG(0.9f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = BG(1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = MED(0.78f);
    style.Colors[ImGuiCol_FrameBgActive] = MED(1.00f);
    style.Colors[ImGuiCol_TitleBg] = LOW(1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = HI(1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = BG(0.75f);
    style.Colors[ImGuiCol_MenuBarBg] = BG(0.47f);
    style.Colors[ImGuiCol_ScrollbarBg] = BG(1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = MED(0.78f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = MED(1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_ButtonHovered] = MED(0.86f);
    style.Colors[ImGuiCol_ButtonActive] = MED(1.00f);
    style.Colors[ImGuiCol_Header] = MED(0.76f);
    style.Colors[ImGuiCol_HeaderHovered] = MED(0.86f);
    style.Colors[ImGuiCol_HeaderActive] = HI(1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    style.Colors[ImGuiCol_ResizeGripHovered] = MED(0.78f);
    style.Colors[ImGuiCol_ResizeGripActive] = MED(1.00f);
    style.Colors[ImGuiCol_PlotLines] = TEXT(0.63f);
    style.Colors[ImGuiCol_PlotLinesHovered] = MED(1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = TEXT(0.63f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = MED(1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = MED(0.43f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = BG(0.73f);

    style.WindowPadding = ImVec2(6, 4);
    style.WindowRounding = 0.0f;
    style.FramePadding = ImVec2(5, 2);
    style.FrameRounding = 3.0f;
    style.ItemSpacing = ImVec2(7, 1);
    style.ItemInnerSpacing = ImVec2(1, 1);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 6.0f;
    style.ScrollbarSize = 12.0f;
    style.ScrollbarRounding = 16.0f;
    style.GrabMinSize = 20.0f;
    style.GrabRounding = 2.0f;

    style.WindowTitleAlign.x = 0.50f;

    style.Colors[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 1.0f;
}

void GraphUI::themeVGUI() {
    // Credit: https://github.com/ocornut/imgui/issues/707#issuecomment-576867100

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.14f, 0.16f, 0.11f, 0.52f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.30f, 0.23f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.34f, 0.26f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.32f, 0.24f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.30f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.23f, 0.27f, 0.21f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
    colors[ImGuiCol_Button] = ImVec4(0.29f, 0.34f, 0.26f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
    colors[ImGuiCol_Header] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.42f, 0.31f, 0.6f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
    colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.19f, 0.23f, 0.18f, 0.00f);  // grip invis
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.54f, 0.57f, 0.51f, 0.78f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.78f, 0.28f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.73f, 0.67f, 0.24f, 1.00f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameBorderSize = 1.0f;
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
}

void GraphUI::onGraphThemeSwitched() {
    m_viewSettings->m_theme = GraphTheme{};
    switch (m_currentGraphTheme) {
        case 0:
            // GraphTheme{} by default is dark mode.
            break;
        case 1:
            graphThemeLight();
            break;
    }

    m_viewSettings->m_shouldFullColorNodes = true;
}

void GraphUI::graphThemeLight() {
    auto& theme = m_viewSettings->m_theme;

    theme.m_backgroundColor = IM_COL32(255, 255, 255, 255);
    theme.m_gridColor = IM_COL32(230, 230, 230, 255);

    theme.m_nodeColor = theme.m_backgroundColor;
    theme.m_nodeOutlineColor = IM_COL32(50, 50, 50, 255);
}

void GraphUI::refreshFilesInFolder(const std::string& folder, std::vector<FileEntry>& fileEntry) {
    std::filesystem::path rootPath{folder};
    for (const auto& entry : std::filesystem::directory_iterator(rootPath)) {
        const auto& path = entry.path();
        if (entry.is_regular_file() && path.extension() == ".bin") {
            fileEntry.emplace_back(path.string(), FileEntry::Type::FILE);
        } else if (entry.is_directory()) {
            fileEntry.emplace_back(path.string(), FileEntry::Type::FOLDER);
        }
    }
}

void GraphUI::drawFileViewHelper(const std::string& folder, std::vector<FileEntry>& fileEntry) {
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
                m_documentHandler->scheduleOpenDocument(entry.m_path);
            }
        }
    }
}
