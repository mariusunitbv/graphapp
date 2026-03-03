module;
#include <pch.h>

module graph_view;

import texture_loader;

#ifdef __EMSCRIPTEN__
#define GLSL_VERSION "#version 300 es\nprecision mediump float;\n"
#else
#define GLSL_VERSION "#version 330 core\n"
#endif

static constexpr ImVec2 toImVec(Vector2D vec) { return ImVec2(vec.m_x, vec.m_y); }

GraphView::~GraphView() {
    if (m_unitbvLogoTexture) {
        TextureLoader::unloadTexture(m_unitbvLogoTexture);
    }
}

void GraphView::initialize(const GraphModel* model, GraphViewModel* viewModel) {
    m_model = model;
    m_viewModel = viewModel;

    initializeTextures();
    initializeGL();
}

void GraphView::onSDLEvent(const SDL_Event& event) {
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
                    m_drawGrid = !m_drawGrid;
                    break;
                case SDLK_N:
                    m_drawNodes = !m_drawNodes;
                    break;
                case SDLK_F12:
                    m_isSettingsOpen = !m_isSettingsOpen;
                    break;
            }

            break;
    }
}

void GraphView::renderUI() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    if (!isFocusOnUI() && m_viewModel->getHoveredNodeIndex() != INVALID_NODE) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
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

    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }

    // drawFileView();
    // drawInspector();
    drawStatusBar();
    drawSettings();

    drawNodesIndexes(drawList);
    drawMinMax(drawList);
    drawSelectBox(drawList);
    drawMousePosition(drawList);
    drawWatermark(drawList);
}

void GraphView::renderScene() {
    drawBackground();
    drawGrid();
    drawNodes();
}

bool GraphView::isFocusOnUI() const {
    return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

int GraphView::getVsyncMode() const {
    // https://wiki.libsdl.org/SDL3/SDL_GL_SetSwapInterval
    if (m_vsyncMode == 2) {
        return -1;
    }

    return m_vsyncMode;
}

void GraphView::initializeTextures() {
    m_unitbvLogoTexture = TextureLoader::loadPNGFile("assets/unitbv.png");
}

void GraphView::initializeGL() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef __EMSCRIPTEN__
    glEnable(GL_PROGRAM_POINT_SIZE);
    glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE, m_pointSizeRange);
#endif

    initializeNodeGL();
    initializeNodeFastGL();
    initializeGridGL();
}

void GraphView::initializeNodeGL() {
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aWorldPos;
        layout(location = 2) in vec4 aColor;
        layout(location = 3) in vec4 aOutlineColor;

        uniform float uNodeRadius;
        uniform vec2 uScreenSize;
        uniform vec2 uCameraPos;
        uniform float uCameraZoom;

        out vec4 vColor;
        out vec4 vOutlineColor;
        out vec2 vTexCoord;
        
        void main() {
            vec2 screenPos = (aWorldPos - uCameraPos) * uCameraZoom + uScreenSize * 0.5;
            vec2 pos = screenPos + aPos * uNodeRadius;
            
            vec2 ndc = (pos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);

            vColor = aColor;
            vOutlineColor = aOutlineColor;

            vTexCoord = aPos * 0.5 + 0.5;
        }
)";

    constexpr auto fragmentShaderSource = GLSL_VERSION R"(
        in vec4 vColor;
        in vec4 vOutlineColor;
        in vec2 vTexCoord;

        out vec4 FragColor;

        uniform sampler2D uTexture;
        uniform float uOutlineThickness;

        void main() {
            vec2 d = vTexCoord - vec2(0.5);
            float dist2 = dot(d, d); 

            const float r = 0.5;
            if (dist2 > r * r) {
                discard;
            }

            if (uOutlineThickness > 0.0) {
                if (dist2 > (r - uOutlineThickness) * (r - uOutlineThickness)) {
                    FragColor = vOutlineColor;
                    return;
                }
            }

            FragColor = vColor;
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

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VisibleNode),
                          (void*)(2 * sizeof(float) + 4 * sizeof(uint8_t)));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);

    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &EBO);
}

void GraphView::initializeNodeFastGL() {
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
        layout(location = 0) in vec2 aWorldPos;
        layout(location = 1) in vec4 aColor;
        layout(location = 2) in vec4 aOutlineColor;

        uniform float uNodeRadius;
        uniform vec2 uScreenSize;
        uniform vec2 uCameraPos;
        uniform float uCameraZoom;

        out vec4 vColor;
        out vec4 vOutlineColor;

        void main() {
            vec2 screenPos = (aWorldPos - uCameraPos) * uCameraZoom + uScreenSize * 0.5;
            vec2 ndc = (screenPos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);
            gl_PointSize = uNodeRadius * 2.0;

            vColor = aColor;
            vOutlineColor = aOutlineColor;
        }
)";

    constexpr auto fragmentShaderSource = GLSL_VERSION R"(
        in vec4 vColor;
        in vec4 vOutlineColor;

        out vec4 FragColor;

        uniform float uOutlineThickness;

        void main() {
            vec2 d = gl_PointCoord - vec2(0.5);
            float dist2 = dot(d,d);

            const float r = 0.5;
            if (dist2 > r * r) {
                discard;
            }

            if (uOutlineThickness > 0.0) {
                if (dist2 > (r - uOutlineThickness) * (r - uOutlineThickness)) {
                    FragColor = vOutlineColor;
                    return;
                }
            }

            FragColor = vColor;
        }
)";

    const auto vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    const auto fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    auto& nodeGL = m_nodeFastGLObject;
    nodeGL.m_shaderProgram = glCreateProgram();

    glAttachShader(nodeGL.m_shaderProgram, vertexShader);
    glAttachShader(nodeGL.m_shaderProgram, fragmentShader);

    glLinkProgram(nodeGL.m_shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &nodeGL.m_VAO);
    glBindVertexArray(nodeGL.m_VAO);

    glGenBuffers(1, &nodeGL.m_instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, nodeGL.m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VisibleNode), (void*)0);
    glVertexAttribDivisor(0, 1);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VisibleNode),
                          (void*)(2 * sizeof(float)));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VisibleNode),
                          (void*)(2 * sizeof(float) + 4 * sizeof(uint8_t)));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
}

void GraphView::initializeGridGL() {
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
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

    constexpr auto fragmentShaderSource = GLSL_VERSION R"(
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
    ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, 12);
    if (ImGui::BeginMainMenuBar()) {
        ImGui::PopStyleVar();
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Center on Node", "C")) {
                m_isCenterOnNodeDialogOpen = true;
            }

            ImGui::MenuItem("Draw Grid", "G", &m_drawGrid);
            ImGui::MenuItem("Draw Min/Max Bounds", nullptr, &m_drawMinMax);
            ImGui::MenuItem("Draw Nodes", "N", &m_drawNodes);
            ImGui::MenuItem("Draw Nodes Outline", nullptr, &m_drawNodesOutline);

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

void GraphView::drawStatusBar() {
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
    std::snprintf(buffer, sizeof(buffer), "Visible nodes: %zu",
                  m_viewModel->getVisibleNodes().size());

    const auto textWidth = ImGui::CalcTextSize(buffer).x;
    if (textWidth * 1.2f < ImGui::GetContentRegionAvail().x) {
        const auto windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX(windowWidth - textWidth - 10.0f);
        ImGui::Text("%s", buffer);
    }

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

void GraphView::drawFileView() {
    if (ImGui::Begin("File View")) {
    }
    ImGui::End();
}

void GraphView::drawInspector() {
    if (ImGui::Begin("Inspector")) {
    }
    ImGui::End();
}

void GraphView::drawSettings() {
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

        bool shouldCondensateNodes = m_viewModel->shouldCondensateNodesLowZoom();
        if (ImGui::Checkbox("Condensate Nodes at Low Zoom", &shouldCondensateNodes)) {
            m_viewModel->setShouldCondensateNodesLowZoom(shouldCondensateNodes);
        }

        if (shouldCondensateNodes) {
            int maxNodesPerCellBase = m_viewModel->getMaxNodesPerCellBase();
            ImGui::TextUnformatted("Max Nodes per Cell Base:");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderInt("##ndpcb", &maxNodesPerCellBase, 10, 700)) {
                maxNodesPerCellBase = std::clamp(maxNodesPerCellBase, 10, 700);
                m_viewModel->setMaxNodesPerCellBase(maxNodesPerCellBase);
            }

            float nodeCondensationFactor = m_viewModel->getNodeCondensationFactor();
            ImGui::TextUnformatted("Node Condensation Zoom Factor:");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##ndcf", &nodeCondensationFactor, 0.05f, 0.7f, "%.2fx")) {
                nodeCondensationFactor = std::clamp(nodeCondensationFactor, 0.05f, 0.7f);
                m_viewModel->setNodeCondensationFactor(nodeCondensationFactor);
            }

            const auto zoom = m_viewModel->getZoomFactor();
            if (zoom <= nodeCondensationFactor) {
                ImGui::Text("Nodes per Cell at current Zoom: %d",
                            static_cast<int>(maxNodesPerCellBase / zoom));
            } else {
                ImGui::TextUnformatted("Nodes are not condensated at current zoom level.");
            }
        }
    } else if (currentTab == 2) {
        ImGui::SeparatorText("Display");

        ImGui::Checkbox("Draw Nodes Fast", &m_drawNodesFast);
        ImGui::Checkbox("Draw Grid", &m_drawGrid);

        ImGui::Checkbox("Draw Min/Max Bounds", &m_drawMinMax);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Draw the min/max bounds of the graph, which is the\nsmallest rectangle that "
                "contains all the nodes.");
        }

        ImGui::Checkbox("Draw Nodes", &m_drawNodes);
        ImGui::Checkbox("Draw Nodes Outline", &m_drawNodesOutline);

        ImGui::TextUnformatted("Outline Thickness:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##outlineThickness", &m_outlineThickness, 1, 4)) {
            m_outlineThickness = std::clamp(m_outlineThickness, 1, 4);
        }

        auto graphZoom = m_viewModel->getZoomFactor();

        ImGui::TextUnformatted("Graph Zoom Factor:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderFloat("##graphZoom", &graphZoom, 0.05f, 5.f, "%.2fx")) {
            graphZoom = std::clamp(graphZoom, 0.05f, 5.f);
            m_viewModel->setZoomFactor(graphZoom);
        }

        ImGui::TextUnformatted("Grid Spacing:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderFloat("##gridSpacing", &m_gridCellSize, NODE_RADIUS, 100.f, "%.2f")) {
            m_gridCellSize = std::clamp(m_gridCellSize, NODE_RADIUS, 100.f);
        }

        ImGui::TextUnformatted("Minimum Zoom to Show Nodes:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##nodeZoom", &m_nodeCutoffZoom, 0, 500, "%d%%")) {
            m_nodeCutoffZoom = std::clamp(m_nodeCutoffZoom, 0, 500);
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void GraphView::drawNodesIndexes(ImDrawList* drawList) {
    if (!shouldDrawNodes()) {
        return;
    }

    const auto zoom = m_viewModel->getZoomFactor();
    if (zoom < 0.72f) {
        return;
    }

    const auto font = ImGui::GetIO().Fonts->Fonts[1];
    const auto& visibleNodes = m_viewModel->getVisibleNodes();
    for (const auto& visibleNode : visibleNodes) {
        char indexLabel[10];

        auto temp = visibleNode.m_index;
        int len = 0;
        do {
            indexLabel[len++] = '0' + (temp % 10);
            temp /= 10;
        } while (temp > 0 && len < static_cast<int>(sizeof(indexLabel) - 1));
        indexLabel[len] = '\0';
        std::reverse(indexLabel, indexLabel + len);

        const auto worldPos = m_viewModel->worldToScreen(visibleNode.m_worldPos);
        const auto baseSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, indexLabel);
        const auto textPos = toImVec(worldPos) - baseSize * 0.5f;

        drawList->AddText(font, font->FontSize, textPos, visibleNode.m_outlineColor, indexLabel);
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

void GraphView::drawWatermark(ImDrawList* drawList) {
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
    const auto worldBounds = m_viewModel->getVisibleRegionWorld(screenExtraPadding);

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

    const auto& gridGL = m_gridGLObject;

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
    const auto zoom = m_viewModel->getZoomFactor();
    const auto radius = NODE_RADIUS * zoom;
    const auto [cameraX, cameraY] = m_viewModel->getCameraPosition();

    auto& visibleNodes = m_viewModel->getVisibleNodes();
    colorVisibleNodes(visibleNodes);

#ifdef __EMSCRIPTEN__
    constexpr auto shouldDrawFast = false;
#else
    const auto shouldDrawFast = 2.f * radius < m_pointSizeRange[1] && m_drawNodesFast;
#endif

    const auto& nodeGL = shouldDrawFast ? m_nodeFastGLObject : m_nodeGLObject;

    glBindVertexArray(nodeGL.m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, nodeGL.m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, visibleNodes.size() * sizeof(VisibleNode), visibleNodes.data(),
                 GL_DYNAMIC_DRAW);

    glUseProgram(nodeGL.m_shaderProgram);
    glUniform1f(glGetUniformLocation(nodeGL.m_shaderProgram, "uNodeRadius"), radius);
    glUniform2f(glGetUniformLocation(nodeGL.m_shaderProgram, "uScreenSize"), width, height);
    glUniform2f(glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraPos"), cameraX, cameraY);
    glUniform1f(glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraZoom"), zoom);
    glUniform1f(glGetUniformLocation(nodeGL.m_shaderProgram, "uOutlineThickness"),
                m_drawNodesOutline ? (m_outlineThickness / 100.f / zoom) : 0.f);

    if (shouldDrawFast) {
        glDrawArraysInstanced(GL_POINTS, 0, 1, (int)visibleNodes.size());
    } else {
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)visibleNodes.size());
    }

    glBindVertexArray(0);
}

void GraphView::colorVisibleNodes(std::vector<VisibleNode>& visibleNodes) {
    for (auto& visibleNode : visibleNodes) {
        visibleNode.m_color = getNodeColor(visibleNode.m_index);
        visibleNode.m_outlineColor = getOutlineColor(visibleNode.m_index);
    }
}

ImU32 GraphView::getNodeColor(NodeIndex_t nodeIndex) const {
    const auto node = m_model->getNode(nodeIndex);

    int nodeAlpha = (m_theme.m_nodeColor >> 24) & 0xFF;
    if (nodeIndex == m_viewModel->getHoveredNodeIndex()) {
        nodeAlpha = std::max(nodeAlpha - 60, 30);
    }

    if (node->hasCustomColor()) {
        return node->getABGR(nodeAlpha);
    }

    return m_theme.m_nodeColor & 0x00FFFFFF | (nodeAlpha << 24);
}

ImU32 GraphView::getOutlineColor(NodeIndex_t nodeIndex) const {
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

    int outlineAlpha = (color >> 24) & 0xFF;
    if (isHovered) {
        outlineAlpha = std::max(outlineAlpha - 60, 30);
    }

    return color | (outlineAlpha << 24);
}

bool GraphView::shouldDrawNodes() const {
    return m_viewModel->getZoomFactor() > (m_nodeCutoffZoom / 100.f) && m_drawNodes;
}
