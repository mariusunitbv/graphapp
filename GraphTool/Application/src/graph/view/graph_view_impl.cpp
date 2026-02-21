module;
#include <pch.h>

module graph_view;

import graph_view_model;

void GraphView::initialize(const GraphModel* model, GraphViewModel* viewModel) {
    m_model = model;
    m_viewModel = viewModel;

    initializeGL();
}

void GraphView::renderUI() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    if (!isFocusOnUI() && m_viewModel->getHoveredNodeIndex() != INVALID_NODE) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }

    drawMenuBar();
    drawDeleteConfirmationDialog();
    drawCenterOnNodeDialog();

    ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (!ImGui::DockBuilderGetNode(dockspaceId)) {
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

        ImGuiID mainDockID = dockspaceId;
        ImGuiID fileViewID{}, inspectorViewID{};
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Left, 0.2f, &fileViewID, &mainDockID);
        ImGui::DockBuilderSplitNode(mainDockID, ImGuiDir_Right, 0.3f, &inspectorViewID, nullptr);

        ImGui::DockBuilderDockWindow("File View", fileViewID);
        ImGui::DockBuilderDockWindow("Inspector", inspectorViewID);
        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::DockSpaceOverViewport(dockspaceId, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

    // We don't need focus the first time the window appears.
    static bool initialized = false;
    if (!initialized) {
        ImGui::SetWindowFocus(nullptr);
        initialized = true;
    }

    drawFileView();
    drawInspector();
    drawStatusBar();
    drawSettings();

    drawNodesIndexes(drawList);
    drawMinMax(drawList);
    drawSelectBox(drawList);
    drawMousePosition(drawList);
}

void GraphView::renderScene() {
    drawBackground();
    drawGrid();
    drawNodes();
}

bool GraphView::isFocusOnUI() const {
    return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

void GraphView::openDeleteConfirmationDialog() { m_isDeleteDialogOpen = true; }

void GraphView::openCenterOnNodeDialog() { m_isCenterOnNodeDialogOpen = true; }

int GraphView::getVsyncMode() const {
    // https://wiki.libsdl.org/SDL3/SDL_GL_SetSwapInterval
    if (m_vsyncMode == 2) {
        return -1;
    }

    return m_vsyncMode;
}

void GraphView::initializeGL() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initializeNodeGL();
    initializeGridGL();
}

void GraphView::initializeNodeGL() {
    constexpr auto vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aWorldPos;
        layout(location = 2) in vec4 aColor;

        uniform float uNodeRadius;
        uniform vec2 uScreenSize;
        uniform vec2 uCameraPos;
        uniform float uCameraZoom;

        out vec4 vColor;
        out vec2 vTexCoord;
        
        void main() {
            vec2 screenPos = (aWorldPos - uCameraPos) * uCameraZoom + uScreenSize * 0.5;
            vec2 pos = screenPos + aPos * uNodeRadius;
            
            vec2 ndc = (pos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);
            vColor = aColor;
            vTexCoord = aPos * 0.5 + 0.5;
        }
)";

    constexpr auto fragmentShaderSource = R"(
        #version 330 core
        in vec4 vColor;
        in vec2 vTexCoord;
        out vec4 FragColor;

        uniform sampler2D uTexture;

        void main() {
            FragColor = texture(uTexture, vTexCoord) * vColor;
        }
)";

    const auto vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    const auto fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    auto& nodeGL = m_nodeGLObject;
    nodeGL.m_shaderProgram = glCreateProgram();

    glAttachShader(nodeGL.m_shaderProgram, vertexShader);
    glAttachShader(nodeGL.m_shaderProgram, fragmentShader);

    glLinkProgram(nodeGL.m_shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    constexpr float quad[] = {-1, -1, 1, -1, 1, 1, -1, 1};
    constexpr uint32_t idx[] = {0, 1, 2, 2, 3, 0};

    glGenVertexArrays(1, &nodeGL.m_VAO);
    glBindVertexArray(nodeGL.m_VAO);

    GLuint quadVBO;
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glGenBuffers(1, &nodeGL.m_instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, nodeGL.m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VisibleNode), (void*)0);
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VisibleNode),
                          (void*)(2 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &EBO);

    m_nodeTexture = TextureLoader::loadPNGFile("assets/node.png");
    m_nodeOutlineTexture = TextureLoader::loadPNGFile("assets/node_outline.png");
}

void GraphView::initializeGridGL() {
    constexpr auto vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aScreenStart;
        layout(location = 2) in vec2 aScreenEnd;
        layout(location = 3) in vec4 aColor;
        layout(location = 4) in float aThickness;

        uniform vec2 uScreenSize;

        out vec4 vColor;
        
        void main() {
            vec2 lineDir = aScreenEnd - aScreenStart;
            vec2 normal = vec2(-lineDir.y, lineDir.x);
            if (length(normal) > 0.0) normal = normalize(normal);

            vec2 offset = normal * aThickness * (aPos.x - 0.5);
            vec2 pos = mix(aScreenStart, aScreenEnd, aPos.y) + offset;

            vec2 ndc = (pos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);
            vColor = aColor;
        }
)";

    constexpr auto fragmentShaderSource = R"(
        #version 330 core
        in vec4 vColor;
        
        out vec4 FragColor;

        void main() {
            FragColor = vColor;
        }
)";

    const auto vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    const auto fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    auto& gridGL = m_gridGLObject;

    gridGL.m_shaderProgram = glCreateProgram();
    glAttachShader(gridGL.m_shaderProgram, vertexShader);
    glAttachShader(gridGL.m_shaderProgram, fragmentShader);

    glLinkProgram(gridGL.m_shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    constexpr float quad[] = {
        -0.5f, 0.f,   // bottom-left
        0.5f,  0.0f,  // bottom-right
        -0.5f, 1.0f,  // top-left
        0.5f,  1.0f   // top-right
    };

    constexpr uint32_t idx[] = {0, 1, 2, 2, 1, 3};

    glGenVertexArrays(1, &gridGL.m_VAO);
    glBindVertexArray(gridGL.m_VAO);

    GLuint quadVBO;
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glGenBuffers(1, &gridGL.m_instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridGL.m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GridLineInstanceData), (void*)0);
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GridLineInstanceData),
                          (void*)(2 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GridLineInstanceData),
                          (void*)(4 * sizeof(float)));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(GridLineInstanceData),
                          (void*)(4 * sizeof(float) + 4 * sizeof(uint8_t)));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);

    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &EBO);
}

GLuint GraphView::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);

        GAPP_THROW(std::string("Shader compilation failed: ") + infoLog);
    }

    return shader;
}

void GraphView::drawMenuBar() {
    auto& displaySafeAreaPadding = ImGui::GetStyle().DisplaySafeAreaPadding;
    auto currentDisplaySafeAreaPadding = displaySafeAreaPadding;
    displaySafeAreaPadding.y = 20.f;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Center on Node", "C")) {
                openCenterOnNodeDialog();
            }

            ImGui::MenuItem("Draw Grid", "G", &m_drawGrid);
            ImGui::MenuItem("Draw Min/Max Bounds", nullptr, &m_drawMinMax);
            ImGui::MenuItem("Draw Nodes", "N", &m_drawNodes);

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
        const auto windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX(windowWidth - textWidth - 10.0f);

        ImGui::Text("%s", buffer);

        ImGui::EndMainMenuBar();
    }

    displaySafeAreaPadding = currentDisplaySafeAreaPadding;
}

void GraphView::drawStatusBar() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));

    const auto [screenWidth, screenHeight] = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, screenHeight - 16));
    ImGui::SetNextWindowSize(ImVec2(screenWidth, 16));
    ImGui::Begin("StatusBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);

    ImGui::Text("Zoom: %d%% %s", (int)std::lround(m_viewModel->getZoomFactor() * 100.f),
                isFocusOnUI() ? "(UNFOCUSED)" : "");
    ImGui::SameLine();

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "Visible nodes: %llu",
                  m_viewModel->getVisibleNodes().size());

    const auto textWidth = ImGui::CalcTextSize(buffer).x;
    const auto windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX(windowWidth - textWidth - 10.0f);

    ImGui::Text("%s", buffer);

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void GraphView::drawDeleteConfirmationDialog() {
    if (!m_isDeleteDialogOpen) {
        return;
    }

    ImGui::OpenPopup("Confirmation");

    const auto centerPos = ImGui::GetIO().DisplaySize * 0.5f;
    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Confirmation", &m_isDeleteDialogOpen,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete %llu node?",
                    m_viewModel->getSelectedNodesCount());
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(140, 0))) {
            m_viewModel->removeSelectedNodes();
            m_isDeleteDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetNavCursorVisible(true);
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        if (ImGui::Button("No", ImVec2(140, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_isDeleteDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void GraphView::drawCenterOnNodeDialog() {
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

void GraphView::drawFileView() {}

void GraphView::drawInspector() {}

void GraphView::drawSettings() {
    if (!m_isSettingsOpen) {
        return;
    }

    const auto& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(700, 400), ImVec2(FLT_MAX, FLT_MAX));

    constexpr const char* settingsTabs[] = {"Appearance", "Video & Performance"};
    static int currentTab = 0;

    constexpr auto windowFlags = ImGuiWindowFlags_NoDocking;
    ImGui::Begin("Settings", &m_isSettingsOpen, windowFlags);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("##settings_tabs", ImVec2(150, 0), ImGuiChildFlags_Borders);
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

        ImGui::TextUnformatted("Graph Background:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto backgroundColor = ImGui::ColorConvertU32ToFloat4(m_theme.m_backgroundColor);
        if (ImGui::ColorEdit4("##bg", (float*)&backgroundColor)) {
            m_theme.m_backgroundColor = ImGui::ColorConvertFloat4ToU32(backgroundColor);
        }

        ImGui::TextUnformatted("Grid Lines:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto gridColor = ImGui::ColorConvertU32ToFloat4(m_theme.m_gridColor);
        if (ImGui::ColorEdit4("##gr", (float*)&gridColor)) {
            m_theme.m_gridColor = ImGui::ColorConvertFloat4ToU32(gridColor);
        }

        ImGui::TextUnformatted("Min/Max Bounds:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto minMaxColor = ImGui::ColorConvertU32ToFloat4(m_theme.m_minMaxColor);
        if (ImGui::ColorEdit4("##mmb", (float*)&minMaxColor)) {
            m_theme.m_minMaxColor = ImGui::ColorConvertFloat4ToU32(minMaxColor);
        }

        ImGui::SeparatorText("Node Appearance");

        ImGui::TextUnformatted("Default Color:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto nodeColor = ImGui::ColorConvertU32ToFloat4(m_theme.m_nodeColor);
        if (ImGui::ColorEdit4("##ndc", (float*)&nodeColor)) {
            m_theme.m_nodeColor = ImGui::ColorConvertFloat4ToU32(nodeColor);
        }

        ImGui::TextUnformatted("Default Outline:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto nodeBorderColor = ImGui::ColorConvertU32ToFloat4(m_theme.m_nodeOutlineColor);
        if (ImGui::ColorEdit4("##ndo", (float*)&nodeBorderColor)) {
            m_theme.m_nodeOutlineColor = ImGui::ColorConvertFloat4ToU32(nodeBorderColor);
        }

        ImGui::TextUnformatted("Selected Outline:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto selectedOutlineColor =
            ImGui::ColorConvertU32ToFloat4(m_theme.m_selectedNodeOutlineColor);
        if (ImGui::ColorEdit4("##so", (float*)&selectedOutlineColor)) {
            m_theme.m_selectedNodeOutlineColor =
                ImGui::ColorConvertFloat4ToU32(selectedOutlineColor);
        }

        ImGui::TextUnformatted("Hovered Outline:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto hoveredOutlineColor =
            ImGui::ColorConvertU32ToFloat4(m_theme.m_hoveredNodeOutlineColor);
        if (ImGui::ColorEdit4("##ho", (float*)&hoveredOutlineColor)) {
            m_theme.m_hoveredNodeOutlineColor = ImGui::ColorConvertFloat4ToU32(hoveredOutlineColor);
        }

        ImGui::TextUnformatted("Selected and Hovered Outline:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto selectedHoveredOutlineColor =
            ImGui::ColorConvertU32ToFloat4(m_theme.m_hoveredAndSelectedNodeOutlineColor);
        if (ImGui::ColorEdit4("##snho", (float*)&selectedHoveredOutlineColor)) {
            m_theme.m_hoveredAndSelectedNodeOutlineColor =
                ImGui::ColorConvertFloat4ToU32(selectedHoveredOutlineColor);
        }
    } else if (currentTab == 1) {
        ImGui::SeparatorText("Video");
        ImGui::Checkbox("Fullscreen Mode", &m_appFullScreen);

        ImGui::Checkbox("Limit FPS", &m_isFpsLimitEnabled);

        ImGui::TextUnformatted("Max FPS:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SliderInt("##fpsLimit", &m_maxFps, 5, 360);

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

        ImGui::SeparatorText("Performance");
        ImGui::Checkbox("Draw Grid", &m_drawGrid);

        ImGui::Checkbox("Draw Min/Max Bounds", &m_drawMinMax);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Draw the min/max bounds of the graph, which is the\nsmallest rectangle that "
                "contains all the nodes.");
        }

        ImGui::Checkbox("Draw Nodes", &m_drawNodes);

        ImGui::SeparatorText("Other");

        auto graphZoom = m_viewModel->getZoomFactor();

        ImGui::TextUnformatted("Graph Zoom Factor:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderFloat("##graphZoom", &graphZoom, 0.01f, 5.f, "%.2fx")) {
            m_viewModel->setZoomFactor(graphZoom);
        }

        ImGui::TextUnformatted("Grid Spacing:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SliderFloat("##gridSpacing", &m_gridCellSize, NODE_RADIUS, 100.f, "%.2f");

        ImGui::TextUnformatted("Minimum Zoom to Show Nodes:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SliderInt("##nodeZoom", &m_nodeCutoffZoom, 0, 500);
    }
    ImGui::EndChild();

    ImGui::End();
}

void GraphView::drawNodesIndexes(ImDrawList* drawList) {
    if (!shouldDrawNodes()) {
        return;
    }

    const auto zoom = m_viewModel->getZoomFactor();
    if (zoom < 0.7f) {
        return;
    }

    const auto fontSize = ImGui::GetFontSize() * zoom;

    const auto& visibleNodes = m_viewModel->getVisibleNodes();
    for (const auto& visibleNode : visibleNodes) {
        const auto node = m_model->getNode(visibleNode.m_index);

        const auto worldPos = m_viewModel->worldToScreen(visibleNode.m_worldPos);
        const auto baseSize = ImGui::CalcTextSize(node->m_labelBuffer);
        const auto scaledSize = baseSize * zoom;
        const auto textPos = toImVec(worldPos) - scaledSize * 0.5f;

        drawList->AddText(nullptr, fontSize, textPos, getOutlineColor(visibleNode.m_index),
                          node->m_labelBuffer);
    }
}

void GraphView::drawMinMax(ImDrawList* drawList) {
    // The HARD limit of the world coordinates, we can draw it to visualize the limits of the graph.
    // This gets drawn regardless of the visible region, because it's useful to see it as a
    // reference when zooming out.
    const auto topLeftBoundsScreen = toImVec(m_viewModel->worldToScreen(WORLD_BOUNDS.m_min));
    const auto bottomRightBoundsScreen = toImVec(m_viewModel->worldToScreen(WORLD_BOUNDS.m_max));

    drawList->AddRect(topLeftBoundsScreen, bottomRightBoundsScreen, m_theme.m_gridColor, 0.f, 0,
                      3.f);

    if (!m_drawMinMax) {
        return;
    }

    const auto& bounds = m_model->getGraphBounds();

    const auto topLeftScreen = toImVec(m_viewModel->worldToScreen(bounds.m_min));
    const auto bottomRightScreen = toImVec(m_viewModel->worldToScreen(bounds.m_max));

    drawList->AddRect(topLeftScreen, bottomRightScreen, m_theme.m_minMaxColor, 0.f, 0, 1.f);
}

void GraphView::drawSelectBox(ImDrawList* drawList) {
    if (!m_viewModel->isSelectingUsingBox()) {
        return;
    }

    const auto& selectBounds = m_viewModel->getSelectBoxBounds();

    const auto topLeft = toImVec(m_viewModel->worldToScreen(selectBounds.m_min));
    const auto bottomRight = toImVec(m_viewModel->worldToScreen(selectBounds.m_max));

    drawList->AddRect(topLeft, bottomRight, IM_COL32(63, 197, 235, 255));
    drawList->AddRectFilled(topLeft + ImVec2(1, 1), bottomRight - ImVec2(1, 1),
                            IM_COL32(53, 187, 225, 50));
}

void GraphView::drawMousePosition(ImDrawList* drawList) {
    if (isFocusOnUI() || m_viewModel->getHoveredNodeIndex() != INVALID_NODE) {
        return;
    }

    const auto [mouseX, mouseY] = ImGui::GetIO().MousePos;
    const auto mouseWorldPos = m_viewModel->screenToWorld({mouseX, mouseY});

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "(%.1f, %.1f)", mouseWorldPos.m_x, mouseWorldPos.m_y);

    drawList->AddText({mouseX + 10.f, mouseY - 10.f}, m_theme.m_nodeOutlineColor, buffer);
}

void GraphView::drawBackground() {
    const auto backgroundColor = ImGui::ColorConvertU32ToFloat4(m_theme.m_backgroundColor);
    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
}

void GraphView::drawGrid() {
    if (!m_drawGrid) {
        return;
    }

    std::vector<GridLineInstanceData> gridLines;

    const auto& io = ImGui::GetIO();
    const auto displaySize = io.DisplaySize;

    const auto screenExtraPadding = Vector2D{m_gridCellSize, m_gridCellSize};
    const auto worldBounds = m_viewModel->getVisibleRegionWorldCoordonates(screenExtraPadding);

    const auto topLeftWorld = worldBounds.m_min;
    const auto bottomRightWorld = worldBounds.m_max;

    const auto topLeftScreen = m_viewModel->worldToScreen(topLeftWorld);
    const auto bottomRightScreen = m_viewModel->worldToScreen(bottomRightWorld);

    const auto originScreen = m_viewModel->worldToScreen({0.f, 0.f});
    gridLines.emplace_back(Vector2D{originScreen.m_x, topLeftScreen.m_y},
                           Vector2D{originScreen.m_x, bottomRightScreen.m_y}, m_theme.m_gridColor,
                           3.5f);
    gridLines.emplace_back(Vector2D{topLeftScreen.m_x, originScreen.m_y},
                           Vector2D{bottomRightScreen.m_x, originScreen.m_y}, m_theme.m_gridColor,
                           3.5f);

    const auto firstVerticalLineX =
        std::floor(topLeftWorld.m_x / m_gridCellSize) * m_gridCellSize + m_gridCellSize;
    for (float x = firstVerticalLineX; x < bottomRightWorld.m_x; x += m_gridCellSize) {
        const auto lineScreenPos = m_viewModel->worldToScreen({x, 0.f});
        gridLines.emplace_back(Vector2D{lineScreenPos.m_x, topLeftScreen.m_y},
                               Vector2D{lineScreenPos.m_x, bottomRightScreen.m_y},
                               m_theme.m_gridColor, 1.f);
    }

    const auto firstHorizontalLineY =
        std::floor(topLeftWorld.m_y / m_gridCellSize) * m_gridCellSize + m_gridCellSize;
    for (float y = firstHorizontalLineY; y < bottomRightWorld.m_y; y += m_gridCellSize) {
        const auto lineScreenPos = m_viewModel->worldToScreen({0.f, y});
        gridLines.emplace_back(Vector2D{topLeftScreen.m_x, lineScreenPos.m_y},
                               Vector2D{bottomRightScreen.m_x, lineScreenPos.m_y},
                               m_theme.m_gridColor, 1.f);
    }

    auto& gridGL = m_gridGLObject;

    glBindVertexArray(gridGL.m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridGL.m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, gridLines.size() * sizeof(GridLineInstanceData), gridLines.data(),
                 GL_DYNAMIC_DRAW);

    glUseProgram(gridGL.m_shaderProgram);
    glUniform2f(glGetUniformLocation(gridGL.m_shaderProgram, "uScreenSize"), displaySize.x,
                displaySize.y);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)gridLines.size());

    glBindVertexArray(0);
}

void GraphView::drawNodes() {
    if (!shouldDrawNodes()) {
        return;
    }

    const auto [width, height] = ImGui::GetIO().DisplaySize;
    const float radius = NODE_RADIUS * m_viewModel->getZoomFactor();
    const auto [cameraX, cameraY] = m_viewModel->getCameraPosition();

    auto& visibleNodes = m_viewModel->getVisibleNodes();
    for (auto& visibleNode : visibleNodes) {
        visibleNode.m_color = getNodeColor(visibleNode.m_index);
    }

    auto& nodeGL = m_nodeGLObject;

    glBindVertexArray(nodeGL.m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, nodeGL.m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, visibleNodes.size() * sizeof(VisibleNode), visibleNodes.data(),
                 GL_DYNAMIC_DRAW);

    glUseProgram(nodeGL.m_shaderProgram);
    glUniform1f(glGetUniformLocation(nodeGL.m_shaderProgram, "uNodeRadius"), radius * 0.95f);
    glUniform2f(glGetUniformLocation(nodeGL.m_shaderProgram, "uScreenSize"), width, height);
    glUniform2f(glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraPos"), cameraX, cameraY);
    glUniform1f(glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraZoom"),
                m_viewModel->getZoomFactor());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_nodeTexture);
    glUniform1i(glGetUniformLocation(nodeGL.m_shaderProgram, "uTexture"), 0);

    // first pass - inner fill
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)visibleNodes.size());

    // second pass - outline
    for (auto& visibleNode : visibleNodes) {
        visibleNode.m_color = getOutlineColor(visibleNode.m_index);
    }

    glBindTexture(GL_TEXTURE_2D, m_nodeOutlineTexture);
    glBufferSubData(GL_ARRAY_BUFFER, 0, visibleNodes.size() * sizeof(VisibleNode),
                    visibleNodes.data());
    glUniform1f(glGetUniformLocation(nodeGL.m_shaderProgram, "uNodeRadius"), radius);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)visibleNodes.size());

    glBindVertexArray(0);
}

ImU32 GraphView::getNodeColor(NodeIndex_t nodeIndex) const {
    const auto node = m_model->getNode(nodeIndex);

    int nodeAlpha = node->hasColor() ? node->m_alpha : ((m_theme.m_nodeColor >> 24) & 0xFF);
    if (nodeIndex == m_viewModel->getHoveredNodeIndex()) {
        nodeAlpha = std::max(nodeAlpha - 60, 30);
    }

    if (!node->hasColor()) {
        return m_theme.m_nodeColor | (nodeAlpha << 24);
    }

    return IM_COL32(node->m_red, node->m_green, node->m_blue, nodeAlpha);
}

ImU32 GraphView::getOutlineColor(NodeIndex_t nodeIndex) const {
    const auto node = m_model->getNode(nodeIndex);

    const auto isHovered = nodeIndex == m_viewModel->getHoveredNodeIndex();
    const auto isSelected = m_viewModel->isNodeSelected(nodeIndex);

    ImU32 color = m_theme.m_nodeOutlineColor;

    if (isSelected && isHovered) {
        color = m_theme.m_hoveredAndSelectedNodeOutlineColor;
    } else if (isHovered) {
        color = m_theme.m_hoveredNodeOutlineColor;
    } else if (isSelected) {
        color = m_theme.m_selectedNodeOutlineColor;
    }

    int nodeAlpha = node->hasColor() ? node->m_alpha : ((color >> 24) & 0xFF);
    if (isHovered) {
        nodeAlpha = std::max(nodeAlpha - 60, 30);
    }

    color = color | (nodeAlpha << 24);
    return color;
}

bool GraphView::shouldDrawNodes() const {
    return m_viewModel->getZoomFactor() > (m_nodeCutoffZoom / 100.f) && m_drawNodes;
}
