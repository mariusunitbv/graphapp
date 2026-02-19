module;
#include <pch.h>

module graph_view;

void GraphView::initialize() { initializeShaders(); }

void GraphView::renderUI() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    const auto& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard && !io.WantCaptureMouse) {
        ImGui::SetMouseCursor(m_currentMouseCursor);
    }

    queryVisible();

    drawMenuBar();
    drawStatusBar();

    removeSelectedNodesPopup();

    if (!m_nodeTexture) {
        m_nodeTexture = TextureLoader::loadPNGFile("assets/node.png");
        m_nodeOutlineTexture = TextureLoader::loadPNGFile("assets/node_outline.png");
    }

    drawGrid(drawList);
    drawQuadTree(drawList, &m_quadTree);
    drawMinMax(drawList);
    drawNodesText(drawList);
    drawDragSelecting(drawList);
    drawMousePosition(drawList);
    drawUnfocusedBackground(drawList);
}

void GraphView::renderScene() { drawNodesGL(); }

void GraphView::onEvent(const SDL_Event& event) {
    const auto& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
        return;
    }

    const auto altPressed = io.KeyAlt;
    const auto shiftPressed = io.KeyShift;

    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT && !altPressed) {
                if (shiftPressed) {
                    m_selectStartPos = m_selectEndPos = {event.button.x, event.button.y};
                    m_isDragSelecting = true;
                } else {
                    onLeftClick(event.button.x, event.button.y);
                }
            }

            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (m_isDragSelecting) {
                    m_isDragSelecting = false;
                }
            }

            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (event.motion.state & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) {
                if (m_isDragSelecting) {
                    m_selectEndPos = {event.motion.x, event.motion.y};
                } else if (m_hoveredNodeIndex != INVALID_NODE) {
                    // GAPP_THROW("Node dragging not implemented yet");
                }
            } else if (event.motion.state & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT) ||
                       (event.motion.state & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) && altPressed) {
                m_viewModel->onCameraPan(-event.motion.xrel, -event.motion.yrel);
            } else {
                markNodeHovered(event.motion.x, event.motion.y);
            }

            break;
        case SDL_EVENT_MOUSE_WHEEL:
            m_viewModel->onCameraZoom(event.wheel.y, event.wheel.mouse_x, event.wheel.mouse_y,
                                      io.DisplaySize.x, io.DisplaySize.y);
            clearVisible();

            break;
        case SDL_EVENT_KEY_DOWN:
            switch (event.key.key) {
                case SDLK_DELETE:
                    if (!m_selectedNodes.empty()) {
                        m_shouldOpenRemoveDialog = true;
                    }

                    break;
                case SDLK_G:
                    m_drawGrid = !m_drawGrid;
                    break;
                case SDLK_N:
                    m_drawNodes = !m_drawNodes;
                    break;
            }

            break;
    }
}

GraphViewModel* GraphView::getViewModel() const { return m_viewModel; }

void GraphView::setViewModel(GraphViewModel* viewModel) { m_viewModel = viewModel; }

GraphModel* GraphView::getModel() const { return m_model; }

void GraphView::setModel(GraphModel* model) {
    m_model = model;

    m_model->setOnAddNodeCallback(this, [](void* instance, const Node* node) {
        reinterpret_cast<GraphView*>(instance)->onNodeAdded(node);
    });
}

GraphTheme& GraphView::getTheme() { return m_theme; }

void GraphView::setTheme(const GraphTheme& theme) { m_theme = theme; }

ImU32 GraphView::getBackgroundColor() const { return m_theme.m_backgroundColor; }

ImVec4 GraphView::getNodeWorldBBox(const Node* node) {
    return getNodeWorldBBox({node->m_x, node->m_y});
}

ImVec4 GraphView::getNodeWorldBBox(const ImVec2& worldPos) {
    return {worldPos.x - NODE_RADIUS, worldPos.y - NODE_RADIUS, worldPos.x + NODE_RADIUS,
            worldPos.y + NODE_RADIUS};
}

void GraphView::buildQuadTree() {
    for (const auto& node : m_model->getNodes()) {
        m_quadTree.insert(&node);
    }
}

void GraphView::initializeShaders() {
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    m_shaderProgram = glCreateProgram();

    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);

    glLinkProgram(m_shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float quad[] = {-1, -1, 1, -1, 1, 1, -1, 1};
    uint32_t idx[] = {0, 1, 2, 2, 3, 0};

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glGenBuffers(1, &m_instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(NodeInstanceData), (void*)0);
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(NodeInstanceData),
                          (void*)(2 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
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

const Node* GraphView::getNode(NodeIndex_t nodeIndex) const {
    return &m_model->getNodes()[nodeIndex];
}

const Node* GraphView::getNode(const ImVec2& screenPos) const {
    NodeIndex_t closestNodeIndex =
        m_quadTree.querySingle(m_model, screenToWorld(screenPos), NODE_RADIUS);
    if (closestNodeIndex != INVALID_NODE) {
        return getNode(closestNodeIndex);
    }

    return nullptr;
}

ImU32 GraphView::getNodeColor(const Node* node) const {
    if (!node->hasColor()) {
        return m_theme.m_nodeColor;
    }

    return IM_COL32(node->m_red, node->m_green, node->m_blue, 255);
}

ImVec2 GraphView::worldToScreen(const ImVec2& worldPos) const {
    const auto& displaySize = ImGui::GetIO().DisplaySize;
    const auto screenPos =
        m_viewModel->worldToScreen(worldPos.x, worldPos.y, displaySize.x, displaySize.y);

    return {screenPos.first, screenPos.second};
}

ImVec2 GraphView::screenToWorld(const ImVec2& screenPos) const {
    const auto& displaySize = ImGui::GetIO().DisplaySize;
    const auto worldPos =
        m_viewModel->screenToWorld(screenPos.x, screenPos.y, displaySize.x, displaySize.y);

    return {worldPos.first, worldPos.second};
}

ImVec4 GraphView::screenAreaToWorld(const ImVec2& screenPadding) const {
    const auto padding = NODE_RADIUS;
    const auto pad = ImVec2{padding, padding} + screenPadding;

    const auto topLeftWorld = screenToWorld(-pad);
    const auto bottomRightWorld = screenToWorld(ImGui::GetIO().DisplaySize + pad);

    return {topLeftWorld.x, topLeftWorld.y, bottomRightWorld.x, bottomRightWorld.y};
}

void GraphView::drawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Draw Grid", "G", &m_drawGrid);
            ImGui::MenuItem("Draw Quad Trees", nullptr, &m_drawQuadTree);
            ImGui::MenuItem("Draw Min/Max Bounds", nullptr, &m_drawMinMax);
            ImGui::MenuItem("Draw Nodes", "N", &m_drawNodes);

            ImGui::EndMenu();
        }

        const auto fps = ImGui::GetIO().Framerate;

        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "FPS: %.1f | %.2f ms", fps, 1000.f / fps);

        const auto textWidth = ImGui::CalcTextSize(buffer).x;
        const auto windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX(windowWidth - textWidth - 10.0f);

        ImGui::Text("%s", buffer);

        ImGui::EndMainMenuBar();
    }
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

    ImGui::Text("Visible nodes: %d", m_visibleNodesThisFrame.size());

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void GraphView::drawGrid(ImDrawList* drawList) {
    if (!m_drawGrid) {
        return;
    }

    const auto& io = ImGui::GetIO();
    const auto displaySize = io.DisplaySize;

    const auto originScreen = worldToScreen({0.f, 0.f});

    drawList->AddLine({originScreen.x, 0.f}, {originScreen.x, displaySize.y}, m_theme.m_gridColor,
                      3.f);
    drawList->AddLine({0.f, originScreen.y}, {displaySize.x, originScreen.y}, m_theme.m_gridColor,
                      3.f);

    const auto topLeftWorld = screenToWorld({0.f, 0.f});
    const auto bottomRightWorld = screenToWorld(displaySize);

    const auto gridSpacing = 100.f;
    const auto firstVerticalLineX = std::floor(topLeftWorld.x / gridSpacing) * gridSpacing;
    for (float x = firstVerticalLineX; x < bottomRightWorld.x; x += gridSpacing) {
        const auto lineScreenPos = worldToScreen({x, 0.f});
        drawList->AddLine({lineScreenPos.x, 0.f}, {lineScreenPos.x, displaySize.y},
                          m_theme.m_gridColor, 1.f);
    }

    const auto firstHorizontalLineY = std::floor(topLeftWorld.y / gridSpacing) * gridSpacing;
    for (float y = firstHorizontalLineY; y < bottomRightWorld.y; y += gridSpacing) {
        const auto lineScreenPos = worldToScreen({0.f, y});
        drawList->AddLine({0.f, lineScreenPos.y}, {displaySize.x, lineScreenPos.y},
                          m_theme.m_gridColor, 1.f);
    }
}

void GraphView::drawQuadTree(ImDrawList* drawList, QuadTree* quadTree) {
    if (!m_drawQuadTree || m_viewModel->getZoomFactor() < 0.5f) {
        return;
    }

    if (!quadTree || !quadTree->validBounds()) {
        return;
    }

    const auto& bounds = quadTree->getBounds();
    if (!intersects(m_visibleRegionArea, bounds)) {
        return;
    }

    const auto topLeftScreen = worldToScreen({bounds.x, bounds.y});
    const auto bottomRightScreen = worldToScreen({bounds.z, bounds.w});

    drawList->AddRect(topLeftScreen, bottomRightScreen, m_theme.m_quadTreeColor, 0.f, 0, 3.f);
    if (quadTree->isSubdivided()) {
        drawQuadTree(drawList, quadTree->getTopLeft());
        drawQuadTree(drawList, quadTree->getTopRight());
        drawQuadTree(drawList, quadTree->getBottomLeft());
        drawQuadTree(drawList, quadTree->getBottomRight());
    }
}

void GraphView::drawMinMax(ImDrawList* drawList) {
    if (!m_drawMinMax || m_minX == std::numeric_limits<float>::max()) {
        return;
    }

    const auto topLeftScreen = worldToScreen({m_minX, m_minY});
    const auto bottomRightScreen = worldToScreen({m_maxX, m_maxY});

    drawList->AddRect(topLeftScreen, bottomRightScreen, m_theme.m_minMaxColor, 0.f, 0, 1.f);
}

void GraphView::drawNodesText(ImDrawList* drawList) {
    if (!shouldDrawNodes()) {
        return;
    }

    const auto zoom = m_viewModel->getZoomFactor();
    if (zoom < 0.7f) {
        return;
    }

    const auto fontSize = ImGui::GetFontSize() * zoom;

    const auto& nodes = m_model->getNodes();
    for (const auto nodeIndex : m_visibleNodesThisFrame) {
        const auto& node = nodes[nodeIndex];

        const auto& borderColor = [this, &node]() {
            const auto isHovered = m_hoveredNodeIndex == node.m_index;
            const auto isSelected = m_selectedNodes.contains(node.m_index);

            if (isHovered && isSelected) {
                return m_theme.m_hoveredAndSelectedNodeBorderColor;
            } else if (isSelected) {
                return m_theme.m_selectedNodeBorderColor;
            } else if (isHovered) {
                return m_theme.m_hoveredNodeBorderColor;
            }

            return m_theme.m_nodeBorderColor;
        }();

        const auto worldPos = worldToScreen(ImVec2(node.m_x, node.m_y));
        if (zoom >= 0.8f) {
            const auto baseSize = ImGui::CalcTextSize(node.m_labelBuffer);
            const auto scaledSize = baseSize * zoom;
            const auto textPos = worldPos - scaledSize * 0.5f;
            drawList->AddText(nullptr, fontSize, textPos, borderColor, node.m_labelBuffer);
        }
    }
}

void GraphView::drawNodesGL() {
    if (!shouldDrawNodes()) {
        return;
    }

    const auto [width, height] = ImGui::GetIO().DisplaySize;
    const float radius = NODE_RADIUS * m_viewModel->getZoomFactor();
    const auto [cameraX, cameraY] = m_viewModel->getCameraPosition();

    for (auto& instance : m_nodeInstances) {
        const auto indexInVisibleNodesVector = std::distance(m_nodeInstances.data(), &instance);
        const auto node = getNode(m_visibleNodesThisFrame[indexInVisibleNodesVector]);
        instance.m_color = getNodeColor(node);
    }

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, m_nodeInstances.size() * sizeof(NodeInstanceData),
                 m_nodeInstances.data(), GL_DYNAMIC_DRAW);

    glUseProgram(m_shaderProgram);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uNodeRadius"), radius * 0.95f);
    glUniform2f(glGetUniformLocation(m_shaderProgram, "uScreenSize"), width, height);
    glUniform2f(glGetUniformLocation(m_shaderProgram, "uCameraPos"), cameraX, cameraY);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uCameraZoom"), m_viewModel->getZoomFactor());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_nodeTexture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "uTexture"), 0);

    // first pass - inner fill
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)m_nodeInstances.size());

    // second pass - outline
    for (auto& instance : m_nodeInstances) {
        const auto indexInVisibleNodesVector = std::distance(m_nodeInstances.data(), &instance);
        const auto nodeIndex = m_visibleNodesThisFrame[indexInVisibleNodesVector];

        const auto& borderColor = [this, &instance, nodeIndex]() {
            const auto isHovered = m_hoveredNodeIndex == nodeIndex;
            const auto isSelected = m_selectedNodes.contains(nodeIndex);

            if (isHovered && isSelected) {
                return m_theme.m_hoveredAndSelectedNodeBorderColor;
            } else if (isSelected) {
                return m_theme.m_selectedNodeBorderColor;
            } else if (isHovered) {
                return m_theme.m_hoveredNodeBorderColor;
            }

            return m_theme.m_nodeBorderColor;
        }();

        instance.m_color = borderColor;
    }

    glBindTexture(GL_TEXTURE_2D, m_nodeOutlineTexture);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_nodeInstances.size() * sizeof(NodeInstanceData),
                    m_nodeInstances.data());
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uNodeRadius"), radius);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)m_nodeInstances.size());

    glBindVertexArray(0);
}

void GraphView::drawDragSelecting(ImDrawList* drawList) {
    if (!m_isDragSelecting) {
        return;
    }

    const auto topLeft = ImVec2(std::min(m_selectStartPos.x, m_selectEndPos.x),
                                std::min(m_selectStartPos.y, m_selectEndPos.y));
    const auto bottomRight = ImVec2(std::max(m_selectStartPos.x, m_selectEndPos.x),
                                    std::max(m_selectStartPos.y, m_selectEndPos.y));

    const auto topLeftWorld = screenToWorld(topLeft);
    const auto bottomRightWorld = screenToWorld(bottomRight);
    const auto selectionArea =
        ImVec4(topLeftWorld.x, topLeftWorld.y, bottomRightWorld.x, bottomRightWorld.y);

    if (!ImGui::GetIO().KeyCtrl) {
        m_selectedNodes.clear();
    }

    const auto queryResult = m_quadTree.query(m_model, selectionArea, m_model->getNodes().size());
    m_selectedNodes.insert(queryResult.begin(), queryResult.end());

    drawList->AddRectFilled(topLeft, bottomRight, ImColor(0, 120, 215, 50));
}

void GraphView::drawMousePosition(ImDrawList* drawList) {
    const auto& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
        return;
    }

    if (m_hoveredNodeIndex != INVALID_NODE) {
        return;
    }

    const auto mouseScreenPos = io.MousePos;

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "(%.1f, %.1f)", screenToWorld(mouseScreenPos).x,
                  screenToWorld(mouseScreenPos).y);

    drawList->AddText({mouseScreenPos.x + 10.f, mouseScreenPos.y - 10.f}, m_theme.m_nodeBorderColor,
                      buffer);
}

void GraphView::drawUnfocusedBackground(ImDrawList* drawList) {
    const auto& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
        drawList->AddRectFilled({0.f, 0.f}, io.DisplaySize, ImColor(0, 0, 0, 100));

        const auto screenCenter = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        const auto topLeft = screenCenter - ImVec2(120.f, 30.f);
        const auto bottomRight = screenCenter + ImVec2(120.f, 30.f);

        const auto unfocusText = "Unfocused - Click to Focus";
        const auto textSize = ImGui::CalcTextSize(unfocusText);
        const auto textPos = screenCenter - textSize * 0.5f;

        drawList->AddRectFilled(topLeft, bottomRight, m_theme.m_nodeColor, 5.f);
        drawList->AddText(textPos, m_theme.m_nodeBorderColor, unfocusText);
    }
}

void GraphView::onNodeAdded(const Node* node) { updateMinMaxBounds(getNodeWorldBBox(node)); }

void GraphView::onLeftClick(float screenX, float screenY) {
    const auto clickedNode = getNode({screenX, screenY});
    if (clickedNode) {
        if (ImGui::GetIO().KeyCtrl) {
            if (m_selectedNodes.contains(clickedNode->m_index)) {
                m_selectedNodes.erase(clickedNode->m_index);
            } else {
                m_selectedNodes.insert(clickedNode->m_index);
            }
        } else {
            m_selectedNodes.clear();
            m_selectedNodes.insert(clickedNode->m_index);
        }

        return;
    }

    if (!m_selectedNodes.empty()) {
        m_selectedNodes.clear();
        return;
    }

    const auto world = screenToWorld({screenX, screenY});
    const auto bbox = ImVec4(world.x - NODE_RADIUS, world.y - NODE_RADIUS, world.x + NODE_RADIUS,
                             world.y + NODE_RADIUS);

    if (updateMinMaxBounds(bbox)) {
        buildQuadTree();
    }

    m_model->addNode(world.x, world.y);
    const auto& lastAddedNode = m_model->getNodes().back();

    m_quadTree.insert(&lastAddedNode);
    m_visibleNodesThisFrame.push_back(lastAddedNode.m_index);
    m_nodeInstances.emplace_back(world.x, world.y, getNodeColor(&lastAddedNode));
}

void GraphView::markNodeHovered(float screenX, float screenY) {
    const auto hoveredNode = getNode({screenX, screenY});
    if (!hoveredNode) {
        m_hoveredNodeIndex = INVALID_NODE;
        m_currentMouseCursor = ImGuiMouseCursor_Arrow;

        return;
    }

    m_hoveredNodeIndex = hoveredNode->m_index;
    m_currentMouseCursor = ImGuiMouseCursor_Hand;
}

void GraphView::queryVisible() {
    m_visibleRegionArea = screenAreaToWorld();
    if (contains(m_lastQueriedQuadTreeArea, m_visibleRegionArea)) {
        return;
    }

    m_lastQueriedQuadTreeArea = screenAreaToWorld(ImGui::GetIO().DisplaySize / 3.f);
    m_visibleNodesThisFrame =
        m_quadTree.query(m_model, m_lastQueriedQuadTreeArea, m_model->getNodes().size());

    m_nodeInstances.clear();
    m_nodeInstances.reserve(m_visibleNodesThisFrame.size());
    for (const auto nodeIndex : m_visibleNodesThisFrame) {
        const auto& node = m_model->getNodes()[nodeIndex];
        m_nodeInstances.emplace_back(node.m_x, node.m_y, IM_COL32(0, 255, 0, 255));
    }
}

void GraphView::clearVisible() {
    m_nodeInstances.clear();
    m_visibleNodesThisFrame.clear();
    m_lastQueriedQuadTreeArea = {
        std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
}

bool GraphView::updateMinMaxBounds(const ImVec4& bbox) {
    static constexpr auto BOUNDS_PADDING = 5.f;

    bool boundsUpdated = false;

    if (bbox.x < m_minX) {
        m_minX = bbox.x - BOUNDS_PADDING;
        boundsUpdated = true;
    }

    if (bbox.z > m_maxX) {
        m_maxX = bbox.z + BOUNDS_PADDING;
        boundsUpdated = true;
    }

    if (bbox.y < m_minY) {
        m_minY = bbox.y - BOUNDS_PADDING;
        boundsUpdated = true;
    }

    if (bbox.w > m_maxY) {
        m_maxY = bbox.w + BOUNDS_PADDING;
        boundsUpdated = true;
    }

    if (boundsUpdated) {
        m_quadTree.setBounds(m_minX, m_minY, m_maxX, m_maxY);
        m_quadTree.clear();
    }

    return boundsUpdated;
}

bool GraphView::shouldDrawNodes() const {
    return m_viewModel->getZoomFactor() >= 0.18f && m_drawNodes;
}

void GraphView::removeSelectedNodesPopup() {
    if (!m_shouldOpenRemoveDialog) {
        return;
    }

    ImGui::OpenPopup("Confirmation");

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(
            "Are you sure you want to delete %llu nodes?\nThis operation cannot be "
            "undone!\n",
            m_selectedNodes.size());
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(140, 0))) {
            ImGui::CloseCurrentPopup();
            removeSelectedNodes();
            m_shouldOpenRemoveDialog = false;
        }

        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(140, 0))) {
            ImGui::CloseCurrentPopup();
            m_shouldOpenRemoveDialog = false;
        }

        ImGui::EndPopup();
    }
}

void GraphView::removeSelectedNodes() {
    // Pre-removal stage.
    m_quadTree.remove(m_model, m_selectedNodes);

    // Actual removal.
    m_model->removeNodes(m_selectedNodes);

    // Post-removal stage.
    m_quadTree.fixIndexesAfterNodeRemoval(m_model);

    m_model->clearIndexRemap();
    m_selectedNodes.clear();
    clearVisible();
}
