module;
#include <pch.h>

module graph_view;

import texture_loader;

#ifdef __EMSCRIPTEN__
#define GLSL_VERSION                                                             \
    "#version 300 es\n#define WEBGL\nprecision mediump float;\nprecision highp " \
    "sampler2D;\nprecision highp usampler2D;\n"
#else
#define GLSL_VERSION "#version 330 core\n"
#endif

static constexpr ImVec2 toImVec(Vector2D vec) { return ImVec2(vec.m_x, vec.m_y); }

GraphView::~GraphView() {
    if (m_unitbvLogoTexture) {
        TextureLoader::unloadTexture(m_unitbvLogoTexture);
    }

    if (m_nodeColorTBO) {
        glDeleteBuffers(1, &m_nodeColorTBO);
    }

    if (m_nodePositionTBO) {
        glDeleteBuffers(1, &m_nodePositionTBO);
    }

    if (m_nodeColorTex) {
        glDeleteTextures(1, &m_nodeColorTex);
    }

    if (m_nodePositionTex) {
        glDeleteTextures(1, &m_nodePositionTex);
    }
}

void GraphView::initialize(const GraphModel* model, GraphViewModel* viewModel) {
    m_model = model;
    m_viewModel = viewModel;

    m_viewModel->addListener(this);

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
                case SDLK_E:
                    m_drawEdges = !m_drawEdges;
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
    drawVersion(drawList);
    drawWatermark(drawList);
}

void GraphView::renderScene() {
    setupNodeBuffers();

    drawBackground();
    drawGrid();
    drawEdges();
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

void GraphView::onFullDataUpdate() {
    m_nodesBufferDirty = m_edgesBufferDirty = m_nodesColorDirty = m_nodesPositionDirty = true;
}

void GraphView::onNodeSelected(NodeIndex_t nodeIndex) { colorNode(nodeIndex); }

void GraphView::onNodeDeselected(NodeIndex_t nodeIndex) { colorNode(nodeIndex); }

void GraphView::onNodeHover(NodeIndex_t nodeIndex) { colorNode(nodeIndex); }

void GraphView::onNodeUnhover(NodeIndex_t nodeIndex) { colorNode(nodeIndex); }

void GraphView::onNodeAdded(NodeIndex_t nodeIndex) {
    m_nodesBufferDirty = m_nodesColorDirty = m_nodesPositionDirty = true;

    colorNode(nodeIndex);
}

void GraphView::onNodeAddedToVisibleData(NodeIndex_t nodeIndex, VisibleData& visibleData) {
    const auto lookupIndex = m_model->getNode(nodeIndex)->getLookupIndex();

    auto& nodeColor = visibleData.m_nodesColors[lookupIndex];
    nodeColor.m_color = getNodeColor(nodeIndex);
    nodeColor.m_outlineColor = getOutlineColor(nodeIndex);
}

void GraphView::initializeTextures() {
    m_unitbvLogoTexture = TextureLoader::loadPNGFile("assets/unitbv.png");
}

void GraphView::initializeGL() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);
    std::cout << "Max texture size: " << m_maxTextureSize << std::endl;

#ifndef __EMSCRIPTEN__
    glEnable(GL_PROGRAM_POINT_SIZE);
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, m_pointSizeRange);
#endif

    initializeEdgeGL();
    initializeNodeGL();
    initializeNodeFastGL();
    initializeNodesBuffersGL();
    initializeGridGL();
}

void GraphView::initializeNodesBuffersGL() {
#ifdef __EMSCRIPTEN__
    glGenTextures(1, &m_nodePositionTex);
    glBindTexture(GL_TEXTURE_2D, m_nodePositionTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &m_nodeColorTex);
    glBindTexture(GL_TEXTURE_2D, m_nodeColorTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glGenBuffers(1, &m_nodePositionTBO);
    glGenTextures(1, &m_nodePositionTex);

    glBindBuffer(GL_TEXTURE_BUFFER, m_nodePositionTBO);
    glBufferData(GL_TEXTURE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, m_nodePositionTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, m_nodePositionTBO);

    glGenBuffers(1, &m_nodeColorTBO);
    glGenTextures(1, &m_nodeColorTex);

    glBindBuffer(GL_TEXTURE_BUFFER, m_nodeColorTBO);
    glBufferData(GL_TEXTURE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, m_nodeColorTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32UI, m_nodeColorTBO);
#endif
}

void GraphView::initializeEdgeGL() {
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
        layout(location = 0) in uint aSrcLookupIndex;
        layout(location = 1) in uint aDestLookupIndex;

#ifdef WEBGL
        uniform sampler2D uNodePositions;
        uniform usampler2D uNodeColors;
        uniform int uTextureWidth;

        vec2 fetchNodePosition(int lookupIndex) {
            int x = lookupIndex % uTextureWidth;
            int y = lookupIndex / uTextureWidth;
            return texelFetch(uNodePositions, ivec2(x, y), 0).xy;
        }

        uvec4 fetchNodeColors(int lookupIndex) {
            int x = lookupIndex % uTextureWidth;
            int y = lookupIndex / uTextureWidth;
            return texelFetch(uNodeColors, ivec2(x, y), 0);
        }
#else
        uniform samplerBuffer uNodePositions;
        uniform usamplerBuffer uNodeColors;

        vec2 fetchNodePosition(int lookupIndex) {
            return texelFetch(uNodePositions, lookupIndex).xy;
        }

        uvec4 fetchNodeColors(int lookupIndex) {
            return texelFetch(uNodeColors, lookupIndex);
        }
#endif

        uniform vec2 uScreenSize;
        uniform vec2 uCameraPos;
        uniform float uCameraZoom;

        out vec4 vColor;

        vec4 unpackColor(uint packedColor) {
            return vec4(
                float(packedColor & 0xFFu) / 255.0,
                float((packedColor >> 8) & 0xFFu) / 255.0,
                float((packedColor >> 16) & 0xFFu) / 255.0,
                float((packedColor >> 24) & 0xFFu) / 255.0
            );
        }

        void main() {
            vec2 worldPos;
            if (gl_VertexID == 0) {
                worldPos = fetchNodePosition(int(aSrcLookupIndex));
                uvec4 colorsPacked = fetchNodeColors(int(aSrcLookupIndex));
                vColor = unpackColor(colorsPacked.y);
            } else {
                worldPos = fetchNodePosition(int(aDestLookupIndex));
                uvec4 colorsPacked = fetchNodeColors(int(aDestLookupIndex));
                vColor = unpackColor(colorsPacked.y);
            }

            vec2 screenPos = (worldPos - uCameraPos) * uCameraZoom + uScreenSize * 0.5;
            vec2 ndc = (screenPos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);
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

    auto& edgeGL = m_edgeGLObject;
    edgeGL.m_shaderProgram = glCreateProgram();

    glAttachShader(edgeGL.m_shaderProgram, vertexShader);
    glAttachShader(edgeGL.m_shaderProgram, fragmentShader);

    glLinkProgram(edgeGL.m_shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &edgeGL.m_VAO);
    glBindVertexArray(edgeGL.m_VAO);

    glGenBuffers(1, &edgeGL.m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, edgeGL.m_VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(VisibleEdge),
                           (void*)offsetof(VisibleEdge, m_startNodeIndexLookup));
    glVertexAttribDivisor(0, 1);

    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(VisibleEdge),
                           (void*)offsetof(VisibleEdge, m_endNodeIndexLookup));
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);

    m_edgeUniforms.m_nodePosition = glGetUniformLocation(edgeGL.m_shaderProgram, "uNodePositions");
    m_edgeUniforms.m_nodeColor = glGetUniformLocation(edgeGL.m_shaderProgram, "uNodeColors");
    m_edgeUniforms.m_screenSize = glGetUniformLocation(edgeGL.m_shaderProgram, "uScreenSize");
    m_edgeUniforms.m_cameraPos = glGetUniformLocation(edgeGL.m_shaderProgram, "uCameraPos");
    m_edgeUniforms.m_cameraZoom = glGetUniformLocation(edgeGL.m_shaderProgram, "uCameraZoom");

#ifdef __EMSCRIPTEN__
    m_edgeUniforms.m_textureWidth = glGetUniformLocation(edgeGL.m_shaderProgram, "uTextureWidth");
#endif
}

void GraphView::initializeNodeGL() {
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in uint aLookupIndex;

        uniform float uNodeRadius;
        uniform vec2 uScreenSize;
        uniform vec2 uCameraPos;
        uniform float uCameraZoom;

#ifdef WEBGL
        uniform sampler2D uNodePositions;
        uniform usampler2D uNodeColors;
        uniform int uTextureWidth;

        vec2 fetchNodePosition(int lookupIndex) {
            int x = lookupIndex % uTextureWidth;
            int y = lookupIndex / uTextureWidth;
            return texelFetch(uNodePositions, ivec2(x, y), 0).xy;
        }

        uvec4 fetchNodeColors(int lookupIndex) {
            int x = lookupIndex % uTextureWidth;
            int y = lookupIndex / uTextureWidth;
            return texelFetch(uNodeColors, ivec2(x, y), 0);
        }
#else
        uniform samplerBuffer uNodePositions;
        uniform usamplerBuffer uNodeColors;

        vec2 fetchNodePosition(int lookupIndex) {
            return texelFetch(uNodePositions, lookupIndex).xy;
        }

        uvec4 fetchNodeColors(int lookupIndex) {
            return texelFetch(uNodeColors, lookupIndex);
        }
#endif

        vec4 unpackColor(uint packedColor) {
            return vec4(
                float(packedColor & 0xFFu) / 255.0,
                float((packedColor >> 8) & 0xFFu) / 255.0,
                float((packedColor >> 16) & 0xFFu) / 255.0,
                float((packedColor >> 24) & 0xFFu) / 255.0
            );
        }

        out vec4 vColor;
        out vec4 vOutlineColor;
        out vec2 vTexCoord;
        
        void main() {
            vec2 worldPos = fetchNodePosition(int(aLookupIndex));

            vec2 screenPos = (worldPos - uCameraPos) * uCameraZoom + uScreenSize * 0.5;
            vec2 pos = screenPos + aPos * uNodeRadius;
            
            vec2 ndc = (pos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);

            uvec4 colorsPacked = fetchNodeColors(int(aLookupIndex));
            uint color = colorsPacked.x;
            uint outlineColor = colorsPacked.y;
            
            vColor = unpackColor(color);
            vOutlineColor = unpackColor(outlineColor);

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

    glGenBuffers(1, &nodeGL.m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, nodeGL.m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glGenBuffers(1, &nodeGL.m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nodeGL.m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glGenBuffers(1, &nodeGL.m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, nodeGL.m_VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(VisibleNode),
                           (void*)offsetof(VisibleNode, m_lookupIndex));
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);

    m_nodeUniforms.m_nodePosition = glGetUniformLocation(nodeGL.m_shaderProgram, "uNodePositions");
    m_nodeUniforms.m_nodeColor = glGetUniformLocation(nodeGL.m_shaderProgram, "uNodeColors");
    m_nodeUniforms.m_nodeRadius = glGetUniformLocation(nodeGL.m_shaderProgram, "uNodeRadius");
    m_nodeUniforms.m_screenSize = glGetUniformLocation(nodeGL.m_shaderProgram, "uScreenSize");
    m_nodeUniforms.m_cameraPos = glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraPos");
    m_nodeUniforms.m_cameraZoom = glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraZoom");
    m_nodeUniforms.m_nodeThickness =
        glGetUniformLocation(nodeGL.m_shaderProgram, "uOutlineThickness");

#ifdef __EMSCRIPTEN__
    m_nodeUniforms.m_textureWidth = glGetUniformLocation(nodeGL.m_shaderProgram, "uTextureWidth");
#endif
}

void GraphView::initializeNodeFastGL() {
#ifndef __EMSCRIPTEN__
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
        layout(location = 0) in uint aLookupIndex;

        uniform samplerBuffer uNodePositions;
        uniform usamplerBuffer uNodeColors;

        uniform float uNodeRadius;
        uniform vec2 uScreenSize;
        uniform vec2 uCameraPos;
        uniform float uCameraZoom;

        out vec4 vColor;
        out vec4 vOutlineColor;

        vec4 unpackColor(uint packedColor) {
            return vec4(
                float(packedColor & 0xFFu) / 255.0,
                float((packedColor >> 8) & 0xFFu) / 255.0,
                float((packedColor >> 16) & 0xFFu) / 255.0,
                float((packedColor >> 24) & 0xFFu) / 255.0
            );
        }

        void main() {
            vec2 worldPos = texelFetch(uNodePositions, int(aLookupIndex)).xy;
            
            uvec4 colorsPacked = texelFetch(uNodeColors, int(aLookupIndex));
            uint color = colorsPacked.x;
            uint outlineColor = colorsPacked.y;

            vec2 screenPos = (worldPos - uCameraPos) * uCameraZoom + uScreenSize * 0.5;
            vec2 ndc = (screenPos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);
            gl_PointSize = uNodeRadius * 2.0;

            vColor = unpackColor(color);
            vOutlineColor = unpackColor(outlineColor);
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

    glGenBuffers(1, &nodeGL.m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, nodeGL.m_VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(VisibleNode), (void*)0);
    glVertexAttribDivisor(0, 1);

    glBindVertexArray(0);

    m_fastNodeUniforms.m_nodePosition =
        glGetUniformLocation(nodeGL.m_shaderProgram, "uNodePositions");
    m_fastNodeUniforms.m_nodeColor = glGetUniformLocation(nodeGL.m_shaderProgram, "uNodeColors");
    m_fastNodeUniforms.m_nodeRadius = glGetUniformLocation(nodeGL.m_shaderProgram, "uNodeRadius");
    m_fastNodeUniforms.m_screenSize = glGetUniformLocation(nodeGL.m_shaderProgram, "uScreenSize");
    m_fastNodeUniforms.m_cameraPos = glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraPos");
    m_fastNodeUniforms.m_cameraZoom = glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraZoom");
    m_fastNodeUniforms.m_nodeThickness =
        glGetUniformLocation(nodeGL.m_shaderProgram, "uOutlineThickness");
#endif
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

    glGenBuffers(1, &gridGL.m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridGL.m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glGenBuffers(1, &gridGL.m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridGL.m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glGenBuffers(1, &gridGL.m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridGL.m_VBO);
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

    m_gridUniformScreenSize = glGetUniformLocation(gridGL.m_shaderProgram, "uScreenSize");
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

            if (ImGui::MenuItem("Refresh Graph", "F5")) {
                m_viewModel->refreshVisibleData();
            }

            ImGui::MenuItem("Render Grid", "G", &m_drawGrid);
            ImGui::MenuItem("Highlight Extents", nullptr, &m_drawMinMax);

            if (ImGui::BeginMenu("Graph Elements")) {
                ImGui::MenuItem("Show Nodes", "N", &m_drawNodes);
                ImGui::MenuItem("Show Nodes Outline", nullptr, &m_drawNodesOutline);
                ImGui::MenuItem("Show Edges", "E", &m_drawEdges);
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

        ImGui::TextUnformatted("Graph Extent:");
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

        if (ImGui::Button("Force Full Update", {-FLT_MIN, 0})) {
            colorNodes();
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
                    "when the zoom level is low enough for condensation to occur.\nA lower value "
                    "means that fewer "
                    "nodes will be drawn in each cell, which\ncan improve performance but may make "
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
                    "Determines at which zoom factor the nodes start to condensate.\nA lower value "
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

        ImGui::Checkbox("Draw Grid", &m_drawGrid);
        ImGui::Checkbox("Draw Graph Extents", &m_drawMinMax);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Show the bounding box of the graph, which is the\n"
                "smallest rectangle containing all node centers.");
        }

        ImGui::Checkbox("Draw Nodes", &m_drawNodes);
        ImGui::Checkbox("Draw Nodes Outline", &m_drawNodesOutline);
        ImGui::Checkbox("Draw Edges", &m_drawEdges);

        ImGui::TextUnformatted("Nodes Radius:");
        ImGui::SetNextItemWidth(-FLT_MIN);

        auto nodeRadius = m_viewModel->getNodesRadius();
        if (ImGui::SliderFloat("##nrs", &nodeRadius, 0.5f, 100.f, "%.2fpx")) {
            nodeRadius = std::clamp(nodeRadius, 0.5f, 100.f);
            m_viewModel->setNodesRadius(nodeRadius);
        }

        ImGui::TextUnformatted("Node Outline Thickness:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##otk", &m_outlineThickness, 1, 4)) {
            m_outlineThickness = std::clamp(m_outlineThickness, 1, 4);
        }

        ImGui::TextUnformatted("Minimum Zoom to Show Nodes:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##nodeZoom", &m_nodeCutoffZoom, 0, 500, "%d%%")) {
            m_nodeCutoffZoom = std::clamp(m_nodeCutoffZoom, 0, 500);
        }

        ImGui::Separator();

        static constexpr const char* fontLabels[] = {"Bigger", "Smaller"};

        ImGui::TextUnformatted("Graph Font Size:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##vsync", fontLabels[m_graphTextFontIndex])) {
            for (int i = 0; i < std::size(fontLabels); ++i) {
                if (ImGui::Selectable(fontLabels[i], m_graphTextFontIndex == i)) {
                    m_graphTextFontIndex = i;
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
        if (ImGui::SliderFloat("##gridSpacing", &m_gridCellSize, 10.f, 100.f, "%.2f")) {
            m_gridCellSize = std::clamp(m_gridCellSize, 10.f, 100.f);
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

    const auto font = ImGui::GetIO().Fonts->Fonts[m_graphTextFontIndex];
    const auto& visibleNodesIndexes = m_viewModel->getVisibleNodesIndexes();
    if (visibleNodesIndexes.empty() || visibleNodesIndexes.size() >= 100'000) {
        return;
    }

    for (uint32_t lookupIndex = static_cast<uint32_t>(visibleNodesIndexes.size());
         lookupIndex-- > 0;) {
        char indexLabel[11];

        const auto index = visibleNodesIndexes[lookupIndex];
        auto temp = index;
        int len = 0;
        do {
            indexLabel[len++] = '0' + (temp % 10);
            temp /= 10;
        } while (temp > 0 && len < static_cast<int>(sizeof(indexLabel) - 1));
        indexLabel[len] = '\0';
        std::reverse(indexLabel, indexLabel + len);

        const auto node = m_model->getNode(index);
        const auto worldPos = m_viewModel->worldToScreen(node->getWorldPos());

        const auto baseSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, indexLabel);
        if (baseSize.x > m_viewModel->getNodesRadius() * 2.f * zoom) {
            continue;
        }

        const auto textPos = toImVec(worldPos) - baseSize * 0.5f;
        drawList->AddText(font, font->FontSize, textPos,
                          m_viewModel->getVisibleNodesColors()[lookupIndex].m_outlineColor,
                          indexLabel);
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

    const auto font = ImGui::GetIO().Fonts->Fonts[m_graphTextFontIndex];

    drawList->AddText(font, font->FontSize, {mouseX + 10.f, mouseY - 10.f},
                      m_theme.m_nodeOutlineColor, buffer);
}

void GraphView::drawVersion(ImDrawList* drawList) {
    const auto font = ImGui::GetIO().Fonts->Fonts[1];

    const auto workPos = ImGui::GetMainViewport()->WorkPos;
    const auto versionSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, GAPP_VERSION);

    drawList->AddRectFilled(workPos + ImVec2{8, 8}, workPos + ImVec2{16, 16} + versionSize,
                            IM_COL32(0, 0, 0, 120), 5.f);
    drawList->AddText(font, font->FontSize, workPos + ImVec2{12, 12}, IM_COL32_WHITE, GAPP_VERSION);
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

void GraphView::setupNodeBuffers() {
    const auto& nodePositions = m_viewModel->getVisibleNodesPositions();
    const auto& nodeColors = m_viewModel->getVisibleNodesColors();

#ifdef __EMSCRIPTEN__
    const int numNodes = static_cast<int>(nodePositions.size());
    const int textureHeight = (numNodes + m_maxTextureSize - 1) / m_maxTextureSize;
#endif

    if (m_nodesPositionDirty) {
#ifdef __EMSCRIPTEN__
        glBindTexture(GL_TEXTURE_2D, m_nodePositionTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, m_maxTextureSize, textureHeight, 0, GL_RG,
                     GL_FLOAT, nodePositions.data());
#else
        glBindBuffer(GL_TEXTURE_BUFFER, m_nodePositionTBO);
        glBufferData(GL_TEXTURE_BUFFER, nodePositions.size() * sizeof(Vector2D),
                     nodePositions.data(), GL_DYNAMIC_DRAW);
#endif

        m_nodesPositionDirty = false;
    }

    if (m_nodesColorDirty) {
#ifdef __EMSCRIPTEN__
        glBindTexture(GL_TEXTURE_2D, m_nodeColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, m_maxTextureSize, textureHeight, 0, GL_RG_INTEGER,
                     GL_UNSIGNED_INT, nodeColors.data());
#else
        glBindBuffer(GL_TEXTURE_BUFFER, m_nodeColorTBO);
        glBufferData(GL_TEXTURE_BUFFER, nodeColors.size() * sizeof(NodeColorInfo),
                     nodeColors.data(), GL_DYNAMIC_DRAW);
#endif

        m_nodesColorDirty = false;
    }
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

    const auto firstVerticalLineX =
        std::floor(topLeftWorld.m_x / m_gridCellSize) * m_gridCellSize + m_gridCellSize;
    for (float x = firstVerticalLineX; x < bottomRightWorld.m_x; x += m_gridCellSize) {
        const auto lineScreenPos = m_viewModel->worldToScreen({x, 0.f});
        const auto thickness = (std::abs(x) < 0.1f) ? 3.5f : 1.f;

        gridLines.emplace_back(Vector2D{lineScreenPos.m_x, topLeftScreen.m_y},
                               Vector2D{lineScreenPos.m_x, bottomRightScreen.m_y},
                               m_theme.m_gridColor, thickness);
    }

    const auto firstHorizontalLineY =
        std::floor(topLeftWorld.m_y / m_gridCellSize) * m_gridCellSize + m_gridCellSize;
    for (float y = firstHorizontalLineY; y < bottomRightWorld.m_y; y += m_gridCellSize) {
        const auto lineScreenPos = m_viewModel->worldToScreen({0.f, y});
        const auto thickness = (std::abs(y) < 0.1f) ? 3.5f : 1.f;

        gridLines.emplace_back(Vector2D{topLeftScreen.m_x, lineScreenPos.m_y},
                               Vector2D{bottomRightScreen.m_x, lineScreenPos.m_y},
                               m_theme.m_gridColor, thickness);
    }

    const auto& gridGL = m_gridGLObject;

    glBindVertexArray(gridGL.m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridGL.m_VBO);
    glBufferData(GL_ARRAY_BUFFER, gridLines.size() * sizeof(GridLineInstanceData), gridLines.data(),
                 GL_DYNAMIC_DRAW);

    glUseProgram(gridGL.m_shaderProgram);
    glUniform2f(m_gridUniformScreenSize, displaySize.x, displaySize.y);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)gridLines.size());

    glBindVertexArray(0);
}

void GraphView::drawEdges() {
    if (!m_drawEdges) {
        return;
    }

    const auto [width, height] = ImGui::GetIO().DisplaySize;
    const auto zoom = m_viewModel->getZoomFactor();
    const auto [cameraX, cameraY] = m_viewModel->getCameraPosition();

    const auto& visibleEdges = m_viewModel->getVisibleEdges();

    const auto& edgeGL = m_edgeGLObject;

    glBindVertexArray(edgeGL.m_VAO);

    if (m_edgesBufferDirty) {
        glBindBuffer(GL_ARRAY_BUFFER, edgeGL.m_VBO);
        if (visibleEdges.size() != m_lastVisibleEdgesCount) {
            glBufferData(GL_ARRAY_BUFFER, visibleEdges.size() * sizeof(VisibleEdge),
                         visibleEdges.data(), GL_DYNAMIC_DRAW);
            m_lastVisibleEdgesCount = static_cast<int>(visibleEdges.size());
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, visibleEdges.size() * sizeof(VisibleEdge),
                            visibleEdges.data());
        }

        m_edgesBufferDirty = false;
    }

    glUseProgram(edgeGL.m_shaderProgram);
    glUniform1i(m_edgeUniforms.m_nodePosition, 0);
    glUniform1i(m_edgeUniforms.m_nodeColor, 1);

    glUniform2f(m_edgeUniforms.m_screenSize, width, height);
    glUniform2f(m_edgeUniforms.m_cameraPos, cameraX, cameraY);
    glUniform1f(m_edgeUniforms.m_cameraZoom, zoom);

#ifdef __EMSCRIPTEN__
    glUniform1i(m_edgeUniforms.m_textureWidth, m_maxTextureSize);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_nodePositionTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_nodeColorTex);
#else
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_nodePositionTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, m_nodeColorTex);
#endif

    glDrawArraysInstanced(GL_LINES, 0, 2, m_lastVisibleEdgesCount);

    glBindVertexArray(0);
}

void GraphView::drawNodes() {
    if (!shouldDrawNodes()) {
        return;
    }

    const auto [width, height] = ImGui::GetIO().DisplaySize;
    const auto zoom = m_viewModel->getZoomFactor();
    const auto radius = m_viewModel->getNodesRadius() * zoom;
    const auto [cameraX, cameraY] = m_viewModel->getCameraPosition();

    const auto& visibleNodes = m_viewModel->getVisibleNodes();

#ifdef __EMSCRIPTEN__
    constexpr auto shouldDrawFast = false;
#else
    const auto shouldDrawFast = 2.f * radius < m_pointSizeRange[1];
    if (m_usedFastDrawingLastFrame != shouldDrawFast) {
        m_nodesBufferDirty = true;
        m_lastVisibleNodesCount = 0;
    }
    m_usedFastDrawingLastFrame = shouldDrawFast;
#endif

    const auto& nodeGL = shouldDrawFast ? m_nodeFastGLObject : m_nodeGLObject;
    const auto& uniforms = shouldDrawFast ? m_fastNodeUniforms : m_nodeUniforms;

    glBindVertexArray(nodeGL.m_VAO);

    if (m_nodesBufferDirty) {
        glBindBuffer(GL_ARRAY_BUFFER, nodeGL.m_VBO);
        if (visibleNodes.size() != m_lastVisibleNodesCount) {
            glBufferData(GL_ARRAY_BUFFER, visibleNodes.size() * sizeof(VisibleNode),
                         visibleNodes.data(), GL_DYNAMIC_DRAW);
            m_lastVisibleNodesCount = static_cast<int>(visibleNodes.size());
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, visibleNodes.size() * sizeof(VisibleNode),
                            visibleNodes.data());
        }

        m_nodesBufferDirty = false;
    }

    glUseProgram(nodeGL.m_shaderProgram);
    glUniform1i(uniforms.m_nodePosition, 0);
    glUniform1i(uniforms.m_nodeColor, 1);

    glUniform1f(uniforms.m_nodeRadius, radius);
    glUniform2f(uniforms.m_screenSize, width, height);
    glUniform2f(uniforms.m_cameraPos, cameraX, cameraY);
    glUniform1f(uniforms.m_cameraZoom, zoom);
    glUniform1f(uniforms.m_nodeThickness, m_drawNodesOutline ? (m_outlineThickness / 100.f) : 0.f);

#ifdef __EMSCRIPTEN__
    glUniform1i(m_nodeUniforms.m_textureWidth, m_maxTextureSize);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_nodePositionTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_nodeColorTex);
#else
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_nodePositionTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, m_nodeColorTex);
#endif

    if (shouldDrawFast) {
        glDrawArraysInstanced(GL_POINTS, 0, 1, m_lastVisibleNodesCount);
    } else {
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, m_lastVisibleNodesCount);
    }

    glBindVertexArray(0);
}

void GraphView::colorNodes() {
    auto& nodeColors = m_viewModel->getVisibleNodesColors();

    for (uint32_t lookupIndex = 0; lookupIndex < nodeColors.size(); ++lookupIndex) {
        const auto nodeIndex = m_viewModel->getVisibleNodesIndexes()[lookupIndex];

        auto& nodeColor = nodeColors[lookupIndex];
        nodeColor.m_color = getNodeColor(nodeIndex);
        nodeColor.m_outlineColor = getOutlineColor(nodeIndex);
    }

    m_nodesColorDirty = true;
}

void GraphView::colorNode(NodeIndex_t nodeIndex) {
    const auto lookupIndex = m_model->getNode(nodeIndex)->getLookupIndex();
    if (!m_viewModel->isValidLookupIndex(nodeIndex, lookupIndex)) {
        return;
    }

    auto& nodeColor = m_viewModel->getVisibleNodesColors()[lookupIndex];
    nodeColor.m_color = getNodeColor(nodeIndex);
    nodeColor.m_outlineColor = getOutlineColor(nodeIndex);

    m_nodesColorDirty = true;
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
