module;
#include <pch.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

module graph_ui;

import texture_loader;
import algorithm;

#ifdef __EMSCRIPTEN__
EM_JS(void, openFileDialog, (GraphDocumentHandler * docHandler), {
    var input = document.createElement('input');
    input.type = 'file';
    input.accept = '.bin';
    input.onchange = function(event) {
        var file = event.target.files[0];
        var reader = new FileReader();
        reader.onload = function(e) {
            var arrayBuffer = e.target.result;
            var data = new Uint8Array(arrayBuffer);

            var ptr = Module._malloc(data.length);
            Module.HEAPU8.set(data, ptr);

            Module.ccall('processFileBuffer', 'void', [ 'number', 'number', 'number' ],
                         [ docHandler, ptr, data.length ]);

            Module._free(ptr);
        };
        reader.readAsArrayBuffer(file);
    };
    input.click();
});

extern "C" {
void processFileBuffer(GraphDocumentHandler* docHandler, const char* data, int size) {
    docHandler->scheduleOpenDocument(data, size);
}
}
#endif

GraphUI::~GraphUI() {
    if (m_unitbvLogoTexture) {
        TextureLoader::unloadTexture(m_unitbvLogoTexture);
    }
}

void GraphUI::initialize(GraphViewSettings* viewSettings, GraphDocumentHandler* docHandler) {
    m_viewSettings = viewSettings;
    m_documentHandler = docHandler;
    m_defaultStyle = ImGui::GetStyle();

    common::Logger::get().addListener(&m_logView);

    initializeTextures();

    m_fileView.initialize(docHandler);
}

void GraphUI::preRenderUpdate(const GraphModel* model, GraphViewModel* viewModel) {
    m_model = model;
    m_viewModel = viewModel;

    static UITheme lastTheme = UITheme::UITHEME_COUNT;
    if (m_currentTheme != lastTheme) {
        onThemeSwitched();
        lastTheme = m_currentTheme;
    }

    static GraphTheme_t lastGraphTheme = static_cast<GraphTheme_t>((int)GraphTheme_t::CUSTOM + 1);
    if (m_currentGraphTheme != lastGraphTheme) {
        onGraphThemeSwitched();
        lastGraphTheme = m_currentGraphTheme;
    }
}

void GraphUI::onSDLEvent(const SDL_Event& event) {
    const auto ctrlPressed = (event.key.mod & SDL_KMOD_CTRL) != 0;
    const auto shiftPressed = (event.key.mod & SDL_KMOD_SHIFT) != 0;

    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_S) {
                if (ctrlPressed && shiftPressed) {
                    saveDocumentAs();
                } else if (ctrlPressed) {
                    saveDocument();
                }
            } else if (event.key.key == SDLK_O && ctrlPressed) {
                openDocument();
            } else if (event.key.key == SDLK_N && ctrlPressed) {
                newDocument();
            } else if (event.key.key == SDLK_W && ctrlPressed) {
                m_documentHandler->scheduleCloseDocument(
                    m_documentHandler->getCurrentOpenedDocumentIndex());
            }

            break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            m_fileView.refreshRootFolder();
            break;
    }

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
                case SDLK_H:
                    m_hideWindows = !m_hideWindows;
                    break;
                case SDLK_N:
                    if (!ctrlPressed) {
                        m_viewSettings->m_drawNodes = !m_viewSettings->m_drawNodes;
                    }
                    break;
                case SDLK_E:
                    if (!ctrlPressed) {
                        m_viewSettings->m_drawEdges = !m_viewSettings->m_drawEdges;
                    } else {
                        m_nodeViewer.openAddEdgePopup();
                    }

                    break;
                case SDLK_F12:
                    m_isSettingsOpen = !m_isSettingsOpen;
                    break;
            }

            break;
    }
}

void GraphUI::render() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    if (!isFocusOnUI() && m_viewModel->getHoveredNodeIndex() != INVALID_NODE) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    drawMenuBar();
    drawDeleteConfirmationDialog();
    drawCenterOnNodeDialog();

    setupDockSpace();

    if (!m_hideWindows) {
        m_fileView.render();
        drawInspector();
        m_nodeViewer.render(m_model, m_viewModel);
        m_logView.render();
        drawAlgorithmsPicker();
        m_pseudocodeView.render(m_model, m_viewModel);
    }

    drawOpenedTabs();
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

void GraphUI::initializeTextures() {
    m_unitbvLogoTexture = TextureLoader::loadPNGFile(Constants::unitbvLogoPath);
}

void GraphUI::setupDockSpace() {
    ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (!ImGui::DockBuilderGetNode(dockspaceId)) {
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

        ImGuiID mainDockID = dockspaceId;
        ImGuiID fileViewID{}, inspectorViewID{}, tabViewID{}, logsViewID{}, nodeViewerID{};
        ImGuiID algorithmPickerID{};

        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Left, 0.23f, &fileViewID, &mainDockID);
        ImGui::DockBuilderSplitNode(fileViewID, ImGuiDir_Down, 0.6f, &algorithmPickerID,
                                    &fileViewID);
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Right, 0.45f, &inspectorViewID,
                                    &mainDockID);
        ImGui::DockBuilderSplitNode(inspectorViewID, ImGuiDir_Up, 0.6f, &inspectorViewID,
                                    &nodeViewerID);
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Up, 0.06f, &tabViewID, nullptr);
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Down, 0.38f, &logsViewID, nullptr);

        ImGui::DockBuilderDockWindow("File View", fileViewID);
        ImGui::DockBuilderDockWindow("Inspector", inspectorViewID);
        ImGui::DockBuilderDockWindow("Node Viewer", nodeViewerID);
        ImGui::DockBuilderDockWindow("Tab Area", tabViewID);
        ImGui::DockBuilderDockWindow("Logs", logsViewID);
        ImGui::DockBuilderDockWindow("Pseudocode", logsViewID);
        ImGui::DockBuilderDockWindow("Algorithms", algorithmPickerID);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    static bool dockNodeFlagsSet = false;
    if (!dockNodeFlagsSet) {
        ImGuiWindow* window = ImGui::FindWindowByName("Tab Area");
        if (window && window->DockNode) {
            ImGuiDockNode* tabNode = window->DockNode;
            if (tabNode) {
                tabNode->LocalFlags |= ImGuiDockNodeFlags_NoUndocking;
                tabNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
                tabNode->LocalFlags |= ImGuiDockNodeFlags_NoDocking;
                tabNode->LocalFlags |= ImGuiDockNodeFlags_NoResizeY;
                dockNodeFlagsSet = true;
            }
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

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                newDocument();
            }

            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                openDocument();
            }

            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                saveDocument();
            }

#ifndef __EMSCRIPTEN__
            if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
                saveDocumentAs();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                SDL_Event quitEvent{};
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            }
#endif

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::BeginDisabled(m_model->getNodeCount() == 0);
            if (ImGui::MenuItem("Center on Node", "C")) {
                m_isCenterOnNodeDialogOpen = true;
            }
            ImGui::EndDisabled();

            if (m_model->getNodeCount() == 0 &&
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("No nodes have been added.");
            }

            if (ImGui::MenuItem("Refresh Graph", "F5")) {
                m_viewModel->refreshVisibleData();
            }

            ImGui::MenuItem("Render Grid", "G", &m_viewSettings->m_drawGrid);
            ImGui::MenuItem("Highlight Extents", nullptr, &m_viewSettings->m_drawMinMax);
            ImGui::MenuItem("Hide UI Windows", "H", &m_hideWindows);

            if (ImGui::BeginMenu("Graph Elements")) {
                ImGui::MenuItem("Show Nodes", "N", &m_viewSettings->m_drawNodes);
                ImGui::MenuItem("Show Nodes Outline", nullptr, &m_viewSettings->m_drawNodesOutline);
                ImGui::MenuItem("Show Edges", "E", &m_viewSettings->m_drawEdges);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("UI Elements")) {
                ImGui::MenuItem("Show File View", nullptr, &m_fileView.isOpen());
                ImGui::MenuItem("Show Inspector", nullptr, &m_inspectorOpen);
                ImGui::MenuItem("Show Node Viewer", nullptr, &m_nodeViewer.isOpen());
                ImGui::MenuItem("Show Logs", nullptr, &m_logView.isOpen());
                ImGui::MenuItem("Show Algorithms", nullptr, &m_algorithmsPickerOpen);
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
                for (auto i = 0u; i < m_graphThemeNames.size(); ++i) {
                    const auto theme = static_cast<GraphTheme_t>(i);
                    const bool selected = (theme == m_currentGraphTheme);
                    if (ImGui::MenuItem(m_graphThemeNames[i].data(), nullptr, selected)) {
                        m_currentGraphTheme = theme;
                    }
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

void GraphUI::drawOpenedTabs() {
    constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("Tab Area", nullptr, flags)) {
        if (ImGui::BeginTabBar("docstab", ImGuiTabBarFlags_Reorderable |
                                              ImGuiTabBarFlags_DrawSelectedOverline |
                                              ImGuiTabBarFlags_FittingPolicyScroll)) {
            const auto currentOpenedDocument = m_documentHandler->getCurrentOpenedDocumentIndex();

            static auto lastOpenedDocument = currentOpenedDocument;
            bool shouldSetCurrentDocument = true;

            if (lastOpenedDocument != currentOpenedDocument) {
                shouldSetCurrentDocument = false;
                lastOpenedDocument = currentOpenedDocument;
            }

            const auto& openDocuments = m_documentHandler->getOpenedDocuments();
            for (size_t i = 0; i < openDocuments.size(); ++i) {
                const auto& doc = openDocuments[i];
                const auto flags = (i == currentOpenedDocument) ? ImGuiTabItemFlags_SetSelected : 0;

                ImGui::PushID(&doc);

                bool isOpen = true;
                if (ImGui::BeginTabItem(doc.getName(), &isOpen, flags)) {
                    if (shouldSetCurrentDocument) {
                        if (currentOpenedDocument != i) {
                            m_documentHandler->scheduleOpenedDocument(i);
                        }
                    }
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

    ImGui::Text("Zoom: %d%%", (int)std::lround(m_viewModel->getZoomFactor() * 100.f));

    if (m_viewModel->isRunningUpdate()) {
        ImGui::SameLine(0, 32.f);
        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(300.0f, barHeight - 3.f),
                           "Building visible data..");
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

    if (m_model->getNodeCount() == 0) {
        m_isCenterOnNodeDialogOpen = false;
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
            if (ImGui::SliderFloat("##graphZoom", &graphZoom, 0.01f, 50.f, "%.2fx")) {
                graphZoom = std::clamp(graphZoom, 0.001f, 50.f);
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

            drawTextCentered("Overscan factor");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Determines how much extra area around the viewport should be rendered.\nA "
                    "higher "
                    "value means that more nodes outside of the viewport will be rendered, which "
                    "can\n"
                    "improve the appearance when moving the viewport around, but may decrease "
                    "performance.");
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            auto overscanFactor = m_viewModel->getOverscanFactor();
            if (ImGui::SliderFloat("##osf", &overscanFactor, 0.f, 2.f, "%.2fx")) {
                overscanFactor = std::clamp(overscanFactor, 0.f, 2.f);
                m_viewModel->setOverscanFactor(overscanFactor);
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
            if (ImGui::SliderFloat("##ndcf", &nodeCondensationFactor, 0.01f, 0.7f, "%.2fx")) {
                nodeCondensationFactor = std::clamp(nodeCondensationFactor, 0.01f, 0.7f);
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

        ImGui::SeparatorText("Graph Statistics");

        if (ImGui::BeginTable(
                "StatsTable", 2,
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Total nodes");

            ImGui::TableSetColumnIndex(1);

            drawTextCentered("%zu", m_model->getNodeCount());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Visible nodes");

            ImGui::TableSetColumnIndex(1);

            drawTextCentered("%zu", m_viewModel->getVisibleNodes().size());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Visible edges");

            ImGui::TableSetColumnIndex(1);

            drawTextCentered("%zu", m_viewModel->getVisibleEdges().size());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            const auto selectedNodesCount = m_viewModel->getSelectedNodesCount();
            drawTextCentered("Selected nodes");

            ImGui::TableSetColumnIndex(1);
            drawTextCentered("%zu", selectedNodesCount);

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

    ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(550, 520), ImGuiCond_FirstUseEver);

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
            if (ImGui::BeginCombo("##graphtheme",
                                  m_graphThemeNames[(size_t)m_currentGraphTheme].data())) {
                for (auto i = 0u; i < m_graphThemeNames.size(); ++i) {
                    const auto theme = static_cast<GraphTheme_t>(i);
                    const bool selected = (theme == m_currentGraphTheme);
                    if (ImGui::Selectable(m_graphThemeNames[i].data(), selected)) {
                        m_currentGraphTheme = theme;
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::EndTable();
        }

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

        ImGui::BeginDisabled(m_currentGraphTheme != GraphTheme_t::CUSTOM);
        ImGui::SeparatorText("Graph Appearance");

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

        ImGui::SeparatorText("Algorithm Visualization");

        auto& algColors = m_viewSettings->m_algorithmColors;
        if (ImGui::BeginTable("AlgorithmVisualizationSettings", 2,
                              ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

            drawColorPicker("Default color", algColors.m_defaultNodeColor);
            drawColorPicker("Visited color", algColors.m_visitedNodeColor);
            drawColorPicker("Currently analyzed color", algColors.m_analyzingNodeColor);
            drawColorPicker("Analyzed color", algColors.m_analyzedNodeColor);
            drawColorPicker("Unreachable color", algColors.m_unreachableNodeColor);
            drawColorPicker("Path color", algColors.m_pathColor);
            drawColorPicker("Relaxed color", algColors.m_relaxedNodeColor);

            ImGui::EndTable();
        }

        if (ImGui::Button("Force Full Update", {-FLT_MIN, 0})) {
            m_viewSettings->m_shouldFullColorNodes = true;
        }
        ImGui::EndDisabled();
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

            const auto labelIndex = (m_vsyncMode == -1) ? 2 : m_vsyncMode;
            if (ImGui::BeginCombo("##vsync", vsyncOptions[labelIndex])) {
                for (int i = 0; i < std::size(vsyncOptions); ++i) {
                    if (ImGui::Selectable(vsyncOptions[i], labelIndex == i)) {
                        if (i == 2) {
                            // https://wiki.libsdl.org/SDL3/SDL_GL_SetSwapInterval
                            m_vsyncMode = -1;
                        } else {
                            m_vsyncMode = i;
                        }
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

void GraphUI::drawAlgorithmsPicker() {
    if (!m_algorithmsPickerOpen) {
        return;
    }

    static IAlgorithm::ExecutionInfo_t algState;

    ImGui::Begin("Algorithms", &m_algorithmsPickerOpen);

    if (m_viewModel->isAlgorithmCreated()) {
        ImGui::SeparatorText("Configuration");

        const auto [selectedNode, _] = m_viewModel->getSelectedNodesPair();
        if (selectedNode != INVALID_NODE) {
            ImGui::Text("Selected node: %u", selectedNode);
        } else {
            ImGui::TextUnformatted("Selected node: None");
        }

        ImGui::BeginDisabled(selectedNode == INVALID_NODE);
        if (ImGui::Button("Set node as source", ImVec2(-FLT_MIN, 0))) {
            m_viewModel->setAlgorithmSourceNode(selectedNode);
        }
        if (ImGui::Button("Set node as target", ImVec2(-FLT_MIN, 0))) {
            m_viewModel->setAlgorithmTargetNode(selectedNode);
        }
        ImGui::EndDisabled();

        ImGui::TextUnformatted("Playback controls");
        ImGui::Separator();

        drawPlaybackControls([&](int pressedButton) {
            switch (pressedButton) {
                case 0:
                    m_viewModel->restartAlgorithm();
                    break;
                case 1:
                    m_viewModel->stepBackwardAlgorithm();
                    break;
                case 2:
                    m_viewModel->stopAlgorithm();
                    break;
                case 3:
                    m_viewModel->toggleAlgorithmPause();
                    break;
                case 4:
                    m_viewModel->stepForwardAlgorithm();
                    break;
                case 5:
                    m_viewModel->finishAlgorithm();
                    break;
            }
        });

        if (ImGui::Button("Benchmark algorithm", ImVec2(-FLT_MIN, 0))) {
            m_viewModel->benchmarkAlgorithm();
        }

        if (ImGui::BeginTable("InspectorTable", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Step delay (ms)");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            auto stepDelayMs = m_viewModel->getAlgorithmStepDelayMs();
            if (ImGui::InputInt("##stepDelay", &stepDelayMs, 50)) {
                stepDelayMs = std::clamp(stepDelayMs, 50, 5000);
                m_viewModel->setAlgorithmStepDelayMs(stepDelayMs);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Iterations per step");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            auto iterationsPerStep = m_viewModel->getAlgorithmIterationsPerStep();
            if (ImGui::InputInt("##iterationsPerStep", &iterationsPerStep, 1)) {
                iterationsPerStep = std::clamp(iterationsPerStep, 1, 50'000);
                m_viewModel->setAlgorithmIterationsPerStep(iterationsPerStep);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Show pseudocode");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##showPseudocode", &m_pseudocodeView.isOpen());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            drawTextCentered("Center on node state change");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##centerOnNodeStateChange",
                            &m_nodeViewer.followNodeStateChangeAlgorithm());

            ImGui::EndTable();
        }

        if (m_model->getNodeCount() < 5'000) {
            algState = m_viewModel->getRunningAlgorithmExecutionInfo();
        } else {
            if (ImGui::Button("Update algorithm state", ImVec2(-FLT_MIN, 0))) {
                algState = m_viewModel->getRunningAlgorithmExecutionInfo();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Updating the algorithm state can be an expensive operation, so it is not\n"
                    "done automatically for large graphs. Click the button to update the state\n"
                    "manually.");
            }
        }

        ImGui::BeginChild("##algs", ImVec2(0, 1000), ImGuiChildFlags_Borders);
        for (auto& [key, value] : algState) {
            if (ImGui::CollapsingHeader(key.c_str())) {
                ImGui::PushID(key.c_str());
                ImGui::InputTextMultiline("##value", &value, ImVec2(-FLT_MIN, 180),
                                          ImGuiInputTextFlags_ReadOnly);
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
    } else {
        algState.clear();

        auto alToInt = [](AlgorithmType type) { return static_cast<int>(type); };
        static auto selectedAlgorithm = alToInt(AlgorithmType::ALGORITHM_TYPE_MAX);

        ImGui::SeparatorText("Algorithms");
        if (ImGui::CollapsingHeader("Traversals")) {
            if (ImGui::TreeNode("What are traversals?")) {
                ImGui::TextWrapped(
                    "Traversal algorithms are used to visit all nodes in a graph "
                    "in a specific order.");

                ImGui::Spacing();

                ImGui::Bullet();
                ImGui::TextLinkOpenURL("Breadth-First Search (BFS)",
                                       "https://en.wikipedia.org/wiki/Breadth-first_search");
                ImGui::SameLine();
                ImGui::TextUnformatted("explores nodes level by level.");

                ImGui::Bullet();
                ImGui::TextLinkOpenURL("Depth-First Search (DFS)",
                                       "https://en.wikipedia.org/wiki/Depth-first_search");
                ImGui::SameLine();
                ImGui::TextUnformatted("explores nodes by going as deep as possible.");

                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::RadioButton("Breadth-First Search", &selectedAlgorithm,
                               alToInt(AlgorithmType::BREADTH_FIRST_SEARCH));
            ImGui::RadioButton("Depth-First Search", &selectedAlgorithm,
                               alToInt(AlgorithmType::DEPTH_FIRST_SEARCH));
        }

        if (ImGui::CollapsingHeader("Pathfinding")) {
            if (ImGui::TreeNode("What is pathfinding?")) {
                ImGui::TextWrapped(
                    "Pathfinding algorithms are used to find the shortest path between two nodes "
                    "in a graph.");

                ImGui::Spacing();
                ImGui::Bullet();
                ImGui::TextLinkOpenURL("Dijkstra's Algorithm",
                                       "https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm");
                ImGui::SameLine();
                ImGui::TextUnformatted(
                    "finds the shortest path from a source node to all other nodes in the graph.");

                ImGui::Spacing();
                ImGui::Bullet();
                ImGui::TextLinkOpenURL("A* Search Algorithm",
                                       "https://en.wikipedia.org/wiki/A*_search_algorithm");
                ImGui::SameLine();
                ImGui::TextUnformatted(
                    "finds the shortest path from a source node to a target node using a "
                    "heuristic.");

                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::RadioButton("Dijkstra", &selectedAlgorithm, alToInt(AlgorithmType::DIJKSTRA));

            ImGui::BeginDisabled(!m_model->hasHeuristic());
            ImGui::RadioButton("A-star", &selectedAlgorithm, alToInt(AlgorithmType::A_STAR));
            if (!m_model->hasHeuristic() &&
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip(
                    "The graph does not have a heuristic, which is required for A-star.\n"
                    "A heuristic is a function that estimates the cost of the cheapest path\n"
                    "from a node to the target node. Without a heuristic, A-star cannot\n"
                    "function properly.");
            }
            ImGui::EndDisabled();

            ImGui::RadioButton("A-Star Landmark", &selectedAlgorithm,
                               alToInt(AlgorithmType::A_STAR_LANDMARK));
        }

        ImGui::SeparatorText("Configuration");

        const auto [src, dest] = m_viewModel->getSelectedNodesPair();
        if (src != INVALID_NODE) {
            ImGui::Text("Source node: %u", src);
        } else {
            ImGui::TextUnformatted("Source node: None");
        }

        if (dest != INVALID_NODE) {
            ImGui::Text("Destination node: %u", dest);
        } else {
            ImGui::TextUnformatted("Destination node: None");
        }

        const auto isTraversal =
            (selectedAlgorithm == alToInt(AlgorithmType::BREADTH_FIRST_SEARCH) ||
             selectedAlgorithm == alToInt(AlgorithmType::DEPTH_FIRST_SEARCH));

        const auto isPathfinding = selectedAlgorithm == alToInt(AlgorithmType::DIJKSTRA) ||
                                   selectedAlgorithm == alToInt(AlgorithmType::A_STAR) ||
                                   selectedAlgorithm == alToInt(AlgorithmType::A_STAR_LANDMARK);

        const char* reasonForDisabling = "";
        bool shouldDisable = [&]() {
            if (selectedAlgorithm == alToInt(AlgorithmType::ALGORITHM_TYPE_MAX)) {
                reasonForDisabling = "No algorithm selected";
                return true;
            }

            if (isTraversal && src == INVALID_NODE) {
                reasonForDisabling = "Select a source node for the traversal";
                return true;
            }

            if (isPathfinding && src == INVALID_NODE) {
                reasonForDisabling = "Select a source node for the pathfinding";
                return true;
            }

            if (isPathfinding && selectedAlgorithm != alToInt(AlgorithmType::DIJKSTRA) &&
                dest == INVALID_NODE) {
                reasonForDisabling = "Select a destination node for the pathfinding";
                return true;
            }

            if (selectedAlgorithm == alToInt(AlgorithmType::A_STAR) && !m_model->hasHeuristic()) {
                reasonForDisabling =
                    "The graph does not have a heuristic, which is required for A-star";
                return true;
            }

            return false;
        }();

        ImGui::BeginDisabled(shouldDisable);
        if (ImGui::Button("Run", ImVec2(-FLT_MIN, 0))) {
            m_pseudocodeView.loadPseudocode(static_cast<AlgorithmType>(selectedAlgorithm));
            m_viewModel->startAlgorithm(static_cast<AlgorithmType>(selectedAlgorithm), src, dest);
        }

        if (shouldDisable && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", reasonForDisabling);
        }

        if (dest != INVALID_NODE && ImGui::Button("Run (Swap sources)", ImVec2(-FLT_MIN, 0))) {
            m_pseudocodeView.loadPseudocode(static_cast<AlgorithmType>(selectedAlgorithm));
            m_viewModel->startAlgorithm(static_cast<AlgorithmType>(selectedAlgorithm), dest, src);
        }

        ImGui::EndDisabled();
    }

    ImGui::End();
}

void GraphUI::drawPlaybackControls(std::function<void(int)> on_click) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGui::BeginChild("##playback_controls", ImVec2(0, 40));

    ImVec2 start = ImGui::GetCursorScreenPos();
    float avail_w = ImGui::GetContentRegionAvail().x;

    const int COUNT = 6;
    const float PADDING_X = 6.0f;
    const float HEIGHT = 32.0f;
    const float spacing = 6.0f;
    const float ROUNDING = 5.0f;

    float BTN_W = (avail_w - PADDING_X * 2.0f - spacing * (COUNT - 1)) / COUNT;
    if (BTN_W < 20.0f) BTN_W = 20.0f;

    float stride = BTN_W + spacing;

    const ImU32 COL_BG = IM_COL32(45, 45, 48, 255);
    const ImU32 COL_BG_HOV = IM_COL32(65, 65, 70, 255);
    const ImU32 COL_BG_ACT = IM_COL32(30, 30, 32, 255);
    const ImU32 COL_ICON = IM_COL32(210, 210, 215, 255);
    const ImU32 COL_ABORT = IM_COL32(200, 70, 70, 255);
    const ImU32 COL_PAUSE_ACT = IM_COL32(90, 180, 255, 255);

    struct Btn {
        const char* id;
    };

    static constexpr Btn btns[6] = {{"##rewind_all"}, {"##step_back"}, {"##abort"},
                                    {"##pause"},      {"##step_fwd"},  {"##skip_end"}};

    for (int i = 0; i < COUNT; i++) {
        ImVec2 p = ImVec2(start.x + PADDING_X + i * stride, start.y);
        ImVec2 p2 = ImVec2(p.x + BTN_W, p.y + HEIGHT);

        ImGui::SetCursorScreenPos(p);
        ImGui::InvisibleButton(btns[i].id, ImVec2(BTN_W, HEIGHT));

        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();
        bool clicked = ImGui::IsItemClicked();

        if (clicked) on_click(i);

        ImU32 bg = active ? COL_BG_ACT : hovered ? COL_BG_HOV : COL_BG;

        dl->AddRectFilled(p, p2, bg, ROUNDING);
        dl->AddRect(p, p2, IM_COL32(80, 80, 85, 255), ROUNDING, 0, 1.0f);

        float cx = p.x + BTN_W * 0.5f;
        float cy = p.y + HEIGHT * 0.5f;
        float s = HEIGHT * 0.22f;

        switch (i) {
            case 0:
                dl->AddRectFilled(ImVec2(cx - s, cy - s), ImVec2(cx - s + 2.5f, cy + s), COL_ICON);
                dl->AddTriangleFilled(ImVec2(cx + s, cy - s), ImVec2(cx + s, cy + s),
                                      ImVec2(cx - s + 3.5f, cy), COL_ICON);
                break;

            case 1:
                dl->AddTriangleFilled(ImVec2(cx + 1.f, cy - s), ImVec2(cx + 1.f, cy + s),
                                      ImVec2(cx - s + 1.f, cy), COL_ICON);
                dl->AddTriangleFilled(ImVec2(cx + s, cy - s), ImVec2(cx + s, cy + s),
                                      ImVec2(cx, cy), COL_ICON);
                break;

            case 2:
                dl->AddRectFilled(ImVec2(cx - s, cy - s), ImVec2(cx + s, cy + s), COL_ABORT, 2.0f);
                break;

            case 3:
                if (m_viewModel->isAlgorithmRunning()) {
                    float bar_w = HEIGHT * 0.08f;
                    float gap = HEIGHT * 0.12f;

                    dl->AddRectFilled(ImVec2(cx - gap - bar_w, cy - s), ImVec2(cx - gap, cy + s),
                                      COL_PAUSE_ACT);
                    dl->AddRectFilled(ImVec2(cx + gap, cy - s), ImVec2(cx + gap + bar_w, cy + s),
                                      COL_PAUSE_ACT);
                } else {
                    dl->AddTriangleFilled(ImVec2(cx - s, cy - s), ImVec2(cx - s, cy + s),
                                          ImVec2(cx + s, cy), COL_ICON);
                }
                break;

            case 4:
                dl->AddTriangleFilled(ImVec2(cx - s, cy - s), ImVec2(cx - s, cy + s),
                                      ImVec2(cx, cy), COL_ICON);
                dl->AddTriangleFilled(ImVec2(cx, cy - s), ImVec2(cx, cy + s), ImVec2(cx + s, cy),
                                      COL_ICON);
                break;

            case 5:
                dl->AddTriangleFilled(ImVec2(cx - s, cy - s), ImVec2(cx - s, cy + s),
                                      ImVec2(cx + s - 3.5f, cy), COL_ICON);
                dl->AddRectFilled(ImVec2(cx + s - 2.5f, cy - s), ImVec2(cx + s, cy + s), COL_ICON);
                break;
        }
    }

    ImGui::EndChild();
}

void GraphUI::drawUnfocusedBackground(ImDrawList* drawList) {
    if (!isFocusOnUI() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) ||
        ImGui::IsAnyItemHovered() || ImGui::IsAnyItemFocused() || ImGui::IsAnyItemActive()) {
        return;
    }

    const auto [cursorPosX, cursorPosY] = ImGui::GetMousePos();
    BoundingBox2D sceneViewBox;
    sceneViewBox.m_min = {m_sceneViewPos.x, m_sceneViewPos.y};
    sceneViewBox.m_max = {m_sceneViewPos.x + m_sceneViewSize.x,
                          m_sceneViewPos.y + m_sceneViewSize.y};

    if (!sceneViewBox.contains({cursorPosX, cursorPosY})) {
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
    if (isFocusOnUI() || m_model->getNodeCount() != 0) {
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
    const auto mousePos = ImGui::GetIO().MousePos;

    const auto font = ImGui::GetIO().Fonts->Fonts[1];
    constexpr auto waterMarkText = "github.com/mariusunitbv/graphapp";
    const auto watermarkSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, waterMarkText);
    const auto textPos = ImVec2{28.f, height - 52.f};

    const auto imagePos = textPos - ImVec2{font->FontSize + 5.f, 0};

    const auto rectMin = imagePos - ImVec2{4, 4};
    const auto rectMax = textPos + watermarkSize + ImVec2{4, 4};

    const auto hovered = (mousePos.x >= rectMin.x && mousePos.x <= rectMax.x &&
                          mousePos.y >= rectMin.y && mousePos.y <= rectMax.y);
    const auto alpha = hovered ? 0.4f : 1.0f;

    drawList->AddRectFilled(rectMin, rectMax, IM_COL32(0, 0, 0, 120 * alpha), 5.f);

    const auto t = static_cast<float>(ImGui::GetTime());
    const auto rainbowColor = IM_COL32((int)((sin(t * 2.0f + 0) * 0.5f + 0.5f) * 255),
                                       (int)((sin(t * 2.0f + 2) * 0.5f + 0.5f) * 255),
                                       (int)((sin(t * 2.0f + 4) * 0.5f + 0.5f) * 255), 255 * alpha);

    drawList->AddImage(m_unitbvLogoTexture, imagePos,
                       imagePos + ImVec2{watermarkSize.y, watermarkSize.y}, {0.f, 0.f}, {1.f, 1.f},
                       rainbowColor);

    const auto black = IM_COL32(0, 0, 0, 255 * alpha);
    drawList->AddText(font, font->FontSize, textPos + ImVec2{-1, 0}, black, waterMarkText);
    drawList->AddText(font, font->FontSize, textPos + ImVec2{1, 0}, black, waterMarkText);
    drawList->AddText(font, font->FontSize, textPos + ImVec2{0, -1}, black, waterMarkText);
    drawList->AddText(font, font->FontSize, textPos + ImVec2{0, 1}, black, waterMarkText);
    drawList->AddText(font, font->FontSize, textPos, IM_COL32(255, 255, 255, 255 * alpha),
                      waterMarkText);
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
    switch (m_currentGraphTheme) {
        case GraphTheme_t::DARK:
            // GraphTheme{} by default is dark mode.
            m_viewSettings->m_theme = GraphTheme{};
            m_viewSettings->m_algorithmColors = AlgorithmColors{};
            break;
        case GraphTheme_t::LIGHT:
            m_viewSettings->m_theme = GraphTheme{};
            m_viewSettings->m_algorithmColors = AlgorithmColors{};
            graphThemeLight();
            break;
        case GraphTheme_t::CUSTOM:
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

void GraphUI::newDocument() { m_documentHandler->scheduleOpenDocument(""); }

void GraphUI::openDocument() {
#ifdef __EMSCRIPTEN__
    openFileDialog(m_documentHandler);
#else
    constexpr const char* filterPatterns[] = {"*.osm", "*.pbf", "*.bin"};
    const auto path = tinyfd_openFileDialog("Open Graph", getOpenedRootFolder().c_str(), 3,
                                            filterPatterns, "Graph Files", false);
    if (path) {
        m_documentHandler->scheduleOpenDocument(path);
    }
#endif
}

void GraphUI::saveDocument() {
#ifdef __EMSCRIPTEN__
    saveDocumentAs();
#else
    const auto& currentDoc = m_documentHandler->getCurrentOpenedDocument();
    if (m_documentHandler->canDirectlySaveCurrentDocument()) {
        m_documentHandler->saveCurrentDocument(currentDoc.m_path);
    } else {
        saveDocumentAs();
    }
#endif
}

void GraphUI::saveDocumentAs() {
#ifdef __EMSCRIPTEN__
    m_documentHandler->saveCurrentDocument("graph.bin");
#else
    constexpr const char* filterPatterns[] = {"*.bin"};
    const auto path = tinyfd_saveFileDialog("Save Graph", getOpenedRootFolder().c_str(), 1,
                                            filterPatterns, "Binary Graph File");
    if (path) {
        m_documentHandler->saveCurrentDocument(path);
    }
#endif
}
