module;
#include <pch.h>

module graph_view;

import graph_model;
import graph_view_model;

void GraphView::initialize(const GraphModel* model, GraphViewModel* viewModel) {
    m_model = model;
    m_viewModel = viewModel;

    initializeGL();
}

void GraphView::renderUI() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    drawMenuBar();
    drawStatusBar();

    drawNodesIndexes(drawList);
    drawQuadTree(drawList, m_model->getQuadTree());
    drawMinMax(drawList);
}

void GraphView::renderScene() {
    drawBackground();
    drawNodes();
}

void GraphView::initializeGL() {
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
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VisibleNode), (void*)0);
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VisibleNode),
                          (void*)(2 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    m_nodeTexture = TextureLoader::loadPNGFile("assets/node.png");
    m_nodeOutlineTexture = TextureLoader::loadPNGFile("assets/node_outline.png");
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

    ImGui::Text("Zoom: %d%%", (int)std::lround(m_viewModel->getZoomFactor() * 100.f));

    ImGui::End();
    ImGui::PopStyleVar(2);
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

    const auto visibleNodes = m_viewModel->getVisibleNodes();
    for (const auto& visibleNode : visibleNodes) {
        const auto node = m_model->getNode(visibleNode.m_index);

        const auto worldPos = m_viewModel->worldToScreen(visibleNode.m_worldPos);
        const auto baseSize = ImGui::CalcTextSize(node->m_labelBuffer);
        const auto scaledSize = baseSize * zoom;
        const auto textPos = toImVec(worldPos) - scaledSize * 0.5f;

        drawList->AddText(nullptr, fontSize, textPos, m_theme.m_nodeBorderColor,
                          node->m_labelBuffer);
    }
}

void GraphView::drawQuadTree(ImDrawList* drawList, const QuadTree* quadTree) {
    if (!m_drawQuadTree || m_viewModel->getZoomFactor() < 0.5f) {
        return;
    }

    if (!quadTree || !quadTree->validBounds()) {
        return;
    }

    const auto& bounds = quadTree->getBounds();
    if (!bounds.intersects(m_viewModel->getVisibleRegion())) {
        return;
    }

    const auto topLeftScreen = toImVec(m_viewModel->worldToScreen(bounds.m_min));
    const auto bottomRightScreen = toImVec(m_viewModel->worldToScreen(bounds.m_max));

    drawList->AddRect(topLeftScreen, bottomRightScreen, m_theme.m_quadTreeColor, 0.f, 0, 3.f);
    if (quadTree->isSubdivided()) {
        drawQuadTree(drawList, quadTree->getTopLeft());
        drawQuadTree(drawList, quadTree->getTopRight());
        drawQuadTree(drawList, quadTree->getBottomLeft());
        drawQuadTree(drawList, quadTree->getBottomRight());
    }
}

void GraphView::drawMinMax(ImDrawList* drawList) {
    if (!m_drawMinMax) {
        return;
    }

    const auto& bounds = m_model->getQuadTree()->getBounds();

    const auto topLeftScreen = toImVec(m_viewModel->worldToScreen(bounds.m_min));
    const auto bottomRightScreen = toImVec(m_viewModel->worldToScreen(bounds.m_max));

    drawList->AddRect(topLeftScreen, bottomRightScreen, m_theme.m_minMaxColor, 0.f, 0, 1.f);
}

void GraphView::drawBackground() {
    const auto backgroundColor = ImGui::ColorConvertU32ToFloat4(m_theme.m_backgroundColor);
    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
}

void GraphView::drawNodes() {
    if (!shouldDrawNodes()) {
        return;
    }

    auto visibleNodes = m_viewModel->getVisibleNodes();

    const auto [width, height] = ImGui::GetIO().DisplaySize;
    const float radius = NODE_RADIUS * m_viewModel->getZoomFactor();
    const auto [cameraX, cameraY] = m_viewModel->getCameraPosition();

    for (auto& visibleNode : visibleNodes) {
        const auto node = m_model->getNode(visibleNode.m_index);
        visibleNode.m_color = getNodeColor(node);
    }

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, visibleNodes.size() * sizeof(VisibleNode), visibleNodes.data(),
                 GL_DYNAMIC_DRAW);

    glUseProgram(m_shaderProgram);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uNodeRadius"), radius * 0.95f);
    glUniform2f(glGetUniformLocation(m_shaderProgram, "uScreenSize"), width, height);
    glUniform2f(glGetUniformLocation(m_shaderProgram, "uCameraPos"), cameraX, cameraY);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uCameraZoom"), m_viewModel->getZoomFactor());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_nodeTexture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "uTexture"), 0);

    // first pass - inner fill
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)visibleNodes.size());

    // second pass - outline
    for (auto& visibleNode : visibleNodes) {
        visibleNode.m_color = m_theme.m_nodeBorderColor;
    }

    glBindTexture(GL_TEXTURE_2D, m_nodeOutlineTexture);
    glBufferSubData(GL_ARRAY_BUFFER, 0, visibleNodes.size() * sizeof(VisibleNode),
                    visibleNodes.data());
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uNodeRadius"), radius);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)visibleNodes.size());

    glBindVertexArray(0);
}

ImU32 GraphView::getNodeColor(const Node* node) const {
    if (!node->hasColor()) {
        return m_theme.m_nodeColor;
    }

    return IM_COL32(node->m_red, node->m_green, node->m_blue, 255);
}

bool GraphView::shouldDrawNodes() const {
    return m_viewModel->getZoomFactor() >= 0.18f && m_drawNodes;
}
