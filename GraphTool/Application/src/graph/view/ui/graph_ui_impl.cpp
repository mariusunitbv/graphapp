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

    initializeTextures();
    refreshRootFolder();
}

void GraphUI::preRenderUpdate(const GraphModel* model, GraphViewModel* viewModel) {
    m_model = model;
    m_viewModel = viewModel;
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

    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }

    drawFileView();
    drawInspector();
    drawOpenedTabs(openDocuments, currentOpenedDocument);
    drawStatusBar();
    drawSettings();

    // We don't need focus the first time the window appears.
    static bool initialized = false;
    if (!initialized) {
        ImGui::SetWindowFocus(nullptr);
        initialized = true;
    }

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
        ImGuiID fileViewID{}, inspectorViewID{}, tabViewID{};
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Left, 0.2f, &fileViewID, &mainDockID);
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Right, 0.3f, &inspectorViewID,
                                    &mainDockID);
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Up, 0.06f, &tabViewID, nullptr);

        ImGui::DockBuilderDockWindow("File View", fileViewID);
        ImGui::DockBuilderDockWindow("Inspector", inspectorViewID);
        ImGui::DockBuilderDockWindow("Tab Area", tabViewID);

        ImGui::DockBuilderFinish(dockspaceId);
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
                ImGui::MenuItem("Show Node Inspector", nullptr, &m_inspectorOpen);
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

            if (ImGui::MenuItem("Demo Window")) {
                m_showDemoWindow = true;
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
        ImGuiDockNode* node = ImGui::GetWindowDockNode();
        if (node) {
            node->LocalFlags = ImGuiDockNodeFlags_NoUndocking;
            node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoDocking |
                                ImGuiDockNodeFlags_NoResizeY;
        }

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
    ImGui::SameLine();

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "N/E: %zu/%zu", m_viewModel->getVisibleNodes().size(),
                  m_viewModel->getVisibleEdges().size());

    const auto textWidth = ImGui::CalcTextSize(buffer).x;
    if (textWidth * 1.2f < ImGui::GetContentRegionAvail().x) {
        const auto windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX(windowWidth - textWidth - 10.0f);
        ImGui::Text("%s", buffer);
    }

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
    }
    ImGui::End();
}

void GraphUI::drawSettings() {
    if (!m_isSettingsOpen) {
        return;
    }

    const auto& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);

    constexpr const char* settingsTabs[] = {"Appearance", "Performance", "Display"};
    static int currentTab = 0;

    ImGui::Begin("Settings", &m_isSettingsOpen);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("##settings_tabs", ImVec2(160, 0), ImGuiChildFlags_Borders);
    if (ImGui::BeginListBox("##tabs", {-FLT_MIN, -FLT_MIN})) {
        for (int i = 0; i < std::size(settingsTabs); ++i) {
            if (ImGui::Selectable(settingsTabs[i], currentTab == i)) {
                currentTab = i;
            }
        }
        ImGui::EndListBox();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::SameLine();

    ImGui::BeginChild("##settings_content", ImVec2(0, 0), ImGuiChildFlags_Borders);
    if (currentTab == 0) {
        ImGui::SeparatorText("Graph Appearance");

        auto& theme = m_viewSettings->m_theme;

        ImGui::TextUnformatted("Graph Background:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto backgroundColor = ImGui::ColorConvertU32ToFloat4(theme.m_backgroundColor);
        if (ImGui::ColorEdit4("##bg", (float*)&backgroundColor)) {
            theme.m_backgroundColor = ImGui::ColorConvertFloat4ToU32(backgroundColor);
        }

        ImGui::TextUnformatted("Grid Lines:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto gridColor = ImGui::ColorConvertU32ToFloat4(theme.m_gridColor);
        if (ImGui::ColorEdit4("##gr", (float*)&gridColor)) {
            theme.m_gridColor = ImGui::ColorConvertFloat4ToU32(gridColor);
        }

        ImGui::TextUnformatted("Graph Extent:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto minMaxColor = ImGui::ColorConvertU32ToFloat4(theme.m_minMaxColor);
        if (ImGui::ColorEdit4("##mmb", (float*)&minMaxColor)) {
            theme.m_minMaxColor = ImGui::ColorConvertFloat4ToU32(minMaxColor);
        }

        ImGui::SeparatorText("Node Appearance");

        ImGui::TextUnformatted("Default Color:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto nodeColor = ImGui::ColorConvertU32ToFloat4(theme.m_nodeColor);
        if (ImGui::ColorEdit4("##ndc", (float*)&nodeColor)) {
            theme.m_nodeColor = ImGui::ColorConvertFloat4ToU32(nodeColor);
        }

        ImGui::TextUnformatted("Default Outline:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto nodeBorderColor = ImGui::ColorConvertU32ToFloat4(theme.m_nodeOutlineColor);
        if (ImGui::ColorEdit4("##ndo", (float*)&nodeBorderColor)) {
            theme.m_nodeOutlineColor = ImGui::ColorConvertFloat4ToU32(nodeBorderColor);
        }

        ImGui::TextUnformatted("Selected Outline:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto selectedOutlineColor =
            ImGui::ColorConvertU32ToFloat4(theme.m_selectedNodeOutlineColor);
        if (ImGui::ColorEdit4("##so", (float*)&selectedOutlineColor)) {
            theme.m_selectedNodeOutlineColor = ImGui::ColorConvertFloat4ToU32(selectedOutlineColor);
        }

        ImGui::TextUnformatted("Hovered Outline:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto hoveredOutlineColor = ImGui::ColorConvertU32ToFloat4(theme.m_hoveredNodeOutlineColor);
        if (ImGui::ColorEdit4("##ho", (float*)&hoveredOutlineColor)) {
            theme.m_hoveredNodeOutlineColor = ImGui::ColorConvertFloat4ToU32(hoveredOutlineColor);
        }

        ImGui::TextUnformatted("Selected and Hovered Outline:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto selectedHoveredOutlineColor =
            ImGui::ColorConvertU32ToFloat4(theme.m_hoveredAndSelectedNodeOutlineColor);
        if (ImGui::ColorEdit4("##snho", (float*)&selectedHoveredOutlineColor)) {
            theme.m_hoveredAndSelectedNodeOutlineColor =
                ImGui::ColorConvertFloat4ToU32(selectedHoveredOutlineColor);
        }

        if (ImGui::Button("Force Full Update", {-FLT_MIN, 0})) {
            m_viewSettings->m_shouldFullColorNodes = true;
        }
    } else if (currentTab == 1) {
        ImGui::SeparatorText("Performance");

#ifndef __EMSCRIPTEN__
        ImGui::Checkbox("Fullscreen Mode", &m_appFullScreen);

        ImGui::Checkbox("Limit FPS", &m_isFpsLimitEnabled);

        ImGui::TextUnformatted("Max FPS:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##fpsLimit", &m_maxFps, 5, 360)) {
            m_maxFps = std::clamp(m_maxFps, 5, 360);
        }
#endif

        constexpr const char* vsyncOptions[] = {"Off", "On", "Adaptive"};

        ImGui::TextUnformatted("VSync Mode:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##vsync", vsyncOptions[m_vsyncMode])) {
            for (int i = 0; i < std::size(vsyncOptions); ++i) {
                if (ImGui::Selectable(vsyncOptions[i], m_vsyncMode == i)) {
                    m_vsyncMode = i;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SeparatorText("Graph Details");

        int maxNodes = m_viewModel->getMaxVisibleNodes();
        ImGui::TextUnformatted("Maximum Visible Nodes:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##mvn", &maxNodes, 10'000, 50'000'000)) {
            maxNodes = std::clamp(maxNodes, 10'000, 50'000'000);
            m_viewModel->setMaxVisibleNodes(maxNodes);
        }

        int edgeDrawPercentage = m_viewModel->getEdgeDrawPercentage();
        ImGui::TextUnformatted("Edges Drawn per Node Percentage:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##edp", &edgeDrawPercentage, 1, 100, "%d%%")) {
            edgeDrawPercentage = std::clamp(edgeDrawPercentage, 1, 100);
            m_viewModel->setEdgeDrawPercentage(edgeDrawPercentage);
        }

        bool shouldCondensateNodes = m_viewModel->shouldCondensateNodesLowZoom();
        if (ImGui::Checkbox("Condensate Nodes at Low Zoom", &shouldCondensateNodes)) {
            m_viewModel->setShouldCondensateNodesLowZoom(shouldCondensateNodes);
        }

        ImGui::Separator();

        if (shouldCondensateNodes) {
            int maxNodesPerCellBase = m_viewModel->getMaxNodesPerCellBase();
            ImGui::TextUnformatted("Nodes Drawn per Cell Percentage:");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "The world is divided into cells of size of %dpx each.\n"
                    "This setting determines how many nodes will be drawn in\neach cell "
                    "when the zoom level is low enough for condensation to occur.\nA lower "
                    "value "
                    "means that fewer "
                    "nodes will be drawn in each cell, which\ncan improve performance but may "
                    "make "
                    "the graph look more sparse.",
                    m_model->getGridMapCellSize());
            }

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderInt("##ndpcb", &maxNodesPerCellBase, 1, 100, "%d%%")) {
                maxNodesPerCellBase = std::clamp(maxNodesPerCellBase, 1, 100);
                m_viewModel->setMaxNodesPerCellBase(maxNodesPerCellBase);
            }

            float nodeCondensationFactor = m_viewModel->getNodeCondensationPercentage();
            ImGui::TextUnformatted("Node Condensation Zoom Factor:");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Determines at which zoom factor the nodes start to condensate.\nA lower "
                    "value "
                    "means that nodes will start to condensate at a lower zoom level.");
            }

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##ndcf", &nodeCondensationFactor, 0.05f, 0.7f, "%.2fx")) {
                nodeCondensationFactor = std::clamp(nodeCondensationFactor, 0.05f, 0.7f);
                m_viewModel->setNodeCondensationPercentage(nodeCondensationFactor);
            }
        }
    } else if (currentTab == 2) {
        ImGui::SeparatorText("Display");

        ImGui::Checkbox("Draw Grid", &m_viewSettings->m_drawGrid);
        ImGui::Checkbox("Draw Graph Extents", &m_viewSettings->m_drawMinMax);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Show the bounding box of the graph, which is the\n"
                "smallest rectangle containing all node centers.");
        }

        ImGui::Checkbox("Draw Nodes", &m_viewSettings->m_drawNodes);
        ImGui::Checkbox("Draw Nodes Outline", &m_viewSettings->m_drawNodesOutline);
        ImGui::Checkbox("Draw Edges", &m_viewSettings->m_drawEdges);

        ImGui::TextUnformatted("Nodes Radius:");
        ImGui::SetNextItemWidth(-FLT_MIN);

        auto nodeRadius = m_viewModel->getNodesRadius();
        if (ImGui::SliderFloat("##nrs", &nodeRadius, 0.5f, 100.f, "%.2fpx")) {
            nodeRadius = std::clamp(nodeRadius, 0.5f, 100.f);
            m_viewModel->setNodesRadius(nodeRadius);
        }

        ImGui::TextUnformatted("Node Outline Thickness:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##otk", &m_viewSettings->m_outlineThickness, 1, 4)) {
            m_viewSettings->m_outlineThickness =
                std::clamp(m_viewSettings->m_outlineThickness, 1, 4);
        }

        ImGui::TextUnformatted("Minimum Zoom to Show Nodes:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##nodeZoom", &m_viewSettings->m_nodeCutoffZoom, 0, 500, "%d%%")) {
            m_viewSettings->m_nodeCutoffZoom = std::clamp(m_viewSettings->m_nodeCutoffZoom, 0, 500);
        }

        ImGui::Separator();

        static constexpr const char* fontLabels[] = {"Bigger", "Smaller"};

        ImGui::TextUnformatted("Graph Font Size:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##vsync", fontLabels[m_viewSettings->m_graphTextFontIndex])) {
            for (int i = 0; i < std::size(fontLabels); ++i) {
                if (ImGui::Selectable(fontLabels[i], m_viewSettings->m_graphTextFontIndex == i)) {
                    m_viewSettings->m_graphTextFontIndex = i;
                }
            }
            ImGui::EndCombo();
        }

        auto graphZoom = m_viewModel->getZoomFactor();

        ImGui::TextUnformatted("Graph Zoom Factor:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderFloat("##graphZoom", &graphZoom, 0.05f, 50.f, "%.2fx")) {
            graphZoom = std::clamp(graphZoom, 0.05f, 50.f);
            m_viewModel->setZoomFactor(graphZoom);
        }

        ImGui::TextUnformatted("Grid Spacing:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderFloat("##gridSpacing", &m_viewSettings->m_gridCellSize, 10.f, 100.f,
                               "%.2f")) {
            m_viewSettings->m_gridCellSize =
                std::clamp(m_viewSettings->m_gridCellSize, 10.f, 100.f);
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void GraphUI::drawVersion(ImDrawList* drawList) {
    const auto font = ImGui::GetIO().Fonts->Fonts[1];

    const auto workPos = ImGui::GetMainViewport()->WorkPos;
    const auto versionSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, GAPP_VERSION);

    drawList->AddRectFilled(workPos + ImVec2{8, 8}, workPos + ImVec2{16, 16} + versionSize,
                            IM_COL32(0, 0, 0, 120), 5.f);
    drawList->AddText(font, font->FontSize, workPos + ImVec2{12, 12}, IM_COL32_WHITE, GAPP_VERSION);
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
