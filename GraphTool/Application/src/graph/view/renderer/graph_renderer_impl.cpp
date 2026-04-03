module;
#include <pch.h>

module graph_renderer;

#ifdef __EMSCRIPTEN__
#define GLSL_VERSION                                                             \
    "#version 300 es\n#define WEBGL\nprecision mediump float;\nprecision highp " \
    "sampler2D;\nprecision highp usampler2D;\n"
#else
#define GLSL_VERSION "#version 330 core\n"
#endif

static constexpr ImVec2 toImVec(Vector2D vec) { return ImVec2(vec.m_x, vec.m_y); }

GraphRenderer::~GraphRenderer() {
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

void GraphRenderer::initialize(const GraphViewSettings* viewSettings) {
    m_viewSettings = viewSettings;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);

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

void GraphRenderer::preRenderUpdate(const GraphModel* model, GraphViewModel* viewModel) {
    m_model = model;
    m_viewModel = viewModel;

    if (m_viewSettings->m_shouldFullColorNodes) {
        colorNodes();
        const_cast<GraphViewSettings*>(m_viewSettings)->m_shouldFullColorNodes = false;
    }
}

void GraphRenderer::render() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    drawCosts(drawList);
    drawNodesIndexes(drawList);
    drawMinMax(drawList);
    drawSelectBox(drawList);
    drawMousePosition(drawList);
}

void GraphRenderer::renderNative() {
    setupNodeBuffers();

    drawBackground();
    drawGrid();
    drawEdges();
    drawNodes();
}

void GraphRenderer::onFullDataUpdate() {
    m_edgesBufferDirty = m_nodesColorDirty = m_nodesPositionDirty = m_selfLoopsDirty = true;
}

void GraphRenderer::onNodeSelected(NodeIndex_t nodeIndex) { colorNode(nodeIndex); }

void GraphRenderer::onNodeDeselected(NodeIndex_t nodeIndex) { colorNode(nodeIndex); }

void GraphRenderer::onNodeHover(NodeIndex_t nodeIndex) { colorNode(nodeIndex); }

void GraphRenderer::onNodeUnhover(NodeIndex_t nodeIndex) { colorNode(nodeIndex); }

void GraphRenderer::onNodeAdded(NodeIndex_t nodeIndex) {
    m_nodesColorDirty = m_nodesPositionDirty = true;

    colorNode(nodeIndex);
}

void GraphRenderer::onNodeAddedToVisibleData(NodeIndex_t nodeIndex, uint32_t lookupIndex,
                                             VisibleData& visibleData) {
    auto& nodeColor = visibleData.m_nodesColors[lookupIndex];
    nodeColor.m_color = getNodeColor(nodeIndex);
    nodeColor.m_outlineColor = getOutlineColor(nodeIndex);
}

void GraphRenderer::initializeNodesBuffersGL() {
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

    glGenTextures(1, &m_selfLoopsTex);
    glBindTexture(GL_TEXTURE_2D, m_selfLoopsTex);

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

    glGenBuffers(1, &m_selfLoopsTBO);
    glGenTextures(1, &m_selfLoopsTex);

    glBindBuffer(GL_TEXTURE_BUFFER, m_selfLoopsTBO);
    glBufferData(GL_TEXTURE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, m_selfLoopsTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, m_selfLoopsTBO);
#endif
}

void GraphRenderer::initializeEdgeGL() {
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
        layout(location = 0) in uint aSrcLookupIndex;
        layout(location = 1) in uint aDestLookupIndexAndFlags;

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
        uniform float uNodeRadius;

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
            float arrowHeadSize = clamp(uNodeRadius * 0.5, 0.5, 20.0);

            uint aDestLookupIndex = aDestLookupIndexAndFlags & 0x7FFFFFFFu;
            uint isBothWay = (aDestLookupIndexAndFlags >> 31) & 1u;

            uvec4 srcColors = fetchNodeColors(int(aSrcLookupIndex));
            uvec4 destColors = fetchNodeColors(int(aDestLookupIndex));

            vec4 srcColor = unpackColor(srcColors.y);
            vec4 destColor = unpackColor(destColors.y);

            vec2 src = fetchNodePosition(int(aSrcLookupIndex));
            vec2 dest = fetchNodePosition(int(aDestLookupIndex));

            vec2 dir = normalize(dest - src);

            if (uNodeRadius > 0.2) {
                src = src + dir * uNodeRadius;
                dest = dest - dir * uNodeRadius;
            }

            vec2 perp = vec2(-dir.y, dir.x);   
            
            uint isDegenerate = 0u;
            vec2 worldPos;
            if (gl_VertexID == 0) {
                worldPos = src;
                vColor = srcColor;
            } else if (gl_VertexID == 1) {
                worldPos = dest;
                vColor = destColor;
            } else if (gl_VertexID == 2) {
                worldPos = dest;
                vColor = destColor;
            } else if (gl_VertexID == 3) {
                vec2 left = dest - dir * arrowHeadSize + perp * arrowHeadSize * 0.5;
                worldPos = left;
                vColor = destColor;
            } else if (gl_VertexID == 4) {
                vec2 right = dest - dir * arrowHeadSize - perp * arrowHeadSize * 0.5;
                worldPos = right;
                vColor = destColor;
            } else if (gl_VertexID == 5) {
                if (isBothWay == 1u) {
                    worldPos = src;
                    vColor = srcColor;
                } else {
                    isDegenerate = 1u;
                }
            } else if (gl_VertexID == 6) {
                if (isBothWay == 1u) {
                    vec2 left = src + dir * arrowHeadSize - perp * arrowHeadSize * 0.5;
                    worldPos = left;
                    vColor = srcColor;
                } else {
                    isDegenerate = 1u;
                }
            } else {
                if (isBothWay == 1u) {
                    vec2 right = src + dir * arrowHeadSize + perp * arrowHeadSize * 0.5;
                    worldPos = right;
                    vColor = srcColor;
                } else {
                    isDegenerate = 1u;
                }
            }

            vec2 screenPos = (worldPos - uCameraPos) * uCameraZoom + uScreenSize * 0.5;
            vec2 ndc = (screenPos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            if (isDegenerate == 1u) {
                ndc = vec2(-2.0, -2.0);
            }

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
                           (void*)offsetof(VisibleEdge, m_endNodeIndexLookupAndBothWayFlag));
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);

    m_edgeUniforms.m_nodePosition = glGetUniformLocation(edgeGL.m_shaderProgram, "uNodePositions");
    m_edgeUniforms.m_nodeColor = glGetUniformLocation(edgeGL.m_shaderProgram, "uNodeColors");
    m_edgeUniforms.m_screenSize = glGetUniformLocation(edgeGL.m_shaderProgram, "uScreenSize");
    m_edgeUniforms.m_cameraPos = glGetUniformLocation(edgeGL.m_shaderProgram, "uCameraPos");
    m_edgeUniforms.m_cameraZoom = glGetUniformLocation(edgeGL.m_shaderProgram, "uCameraZoom");
    m_edgeUniforms.m_nodeRadius = glGetUniformLocation(edgeGL.m_shaderProgram, "uNodeRadius");

#ifdef __EMSCRIPTEN__
    m_edgeUniforms.m_textureWidth = glGetUniformLocation(edgeGL.m_shaderProgram, "uTextureWidth");
#endif
}

void GraphRenderer::initializeNodeGL() {
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
        layout(location = 0) in vec2 aPos;

        uniform float uNodeRadius;
        uniform vec2 uScreenSize;
        uniform vec2 uCameraPos;
        uniform float uCameraZoom;

#ifdef WEBGL
        uniform sampler2D uNodePositions;
        uniform usampler2D uNodeColors;
        uniform usampler2D uSelfLoops;
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

        uint fetchSelfLoop(int lookupIndex) {
            int indexInVector = lookupIndex / 8;
            int bitOffset = lookupIndex % 8;

            int x = indexInVector % uTextureWidth;
            int y = indexInVector / uTextureWidth;

            return (texelFetch(uSelfLoops, ivec2(x, y), 0).r >> uint(bitOffset)) & 1u;
        }
#else
        uniform samplerBuffer uNodePositions;
        uniform usamplerBuffer uNodeColors;
        uniform usamplerBuffer uSelfLoops;

        vec2 fetchNodePosition(int lookupIndex) {
            return texelFetch(uNodePositions, lookupIndex).xy;
        }

        uvec4 fetchNodeColors(int lookupIndex) {
            return texelFetch(uNodeColors, lookupIndex);
        }

        uint fetchSelfLoop(int lookupIndex) {
            int indexInVector = lookupIndex / 8;
            int bitOffset = lookupIndex % 8;

            return (texelFetch(uSelfLoops, indexInVector).r >> uint(bitOffset)) & 1u;
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
        flat out uint vHasSelfLoop;
        
        void main() {
            vec2 worldPos = fetchNodePosition(int(gl_InstanceID));

            vec2 screenPos = (worldPos - uCameraPos) * uCameraZoom + uScreenSize * 0.5;
            vec2 pos = screenPos + aPos * uNodeRadius;
            
            vec2 ndc = (pos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);

            uvec4 colorsPacked = fetchNodeColors(int(gl_InstanceID));
            uint color = colorsPacked.x;
            uint outlineColor = colorsPacked.y;
            
            vColor = unpackColor(color);
            vOutlineColor = unpackColor(outlineColor);

            vTexCoord = aPos * 0.5 + 0.5;
            vHasSelfLoop = fetchSelfLoop(int(gl_InstanceID));
        }
)";

    constexpr auto fragmentShaderSource = GLSL_VERSION R"(
        in vec4 vColor;
        in vec4 vOutlineColor;
        in vec2 vTexCoord;
        flat in uint vHasSelfLoop;

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

            if (vHasSelfLoop == 1u) {
                const float innerR = 0.38;
                const float outerR = 0.4;
                
                if (dist2 > innerR * innerR && dist2 < outerR * outerR) {
                    FragColor = vOutlineColor;
                    return;
                }  
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

    glBindVertexArray(0);

    m_nodeUniforms.m_nodePosition = glGetUniformLocation(nodeGL.m_shaderProgram, "uNodePositions");
    m_nodeUniforms.m_nodeColor = glGetUniformLocation(nodeGL.m_shaderProgram, "uNodeColors");
    m_nodeUniforms.m_nodeRadius = glGetUniformLocation(nodeGL.m_shaderProgram, "uNodeRadius");
    m_nodeUniforms.m_screenSize = glGetUniformLocation(nodeGL.m_shaderProgram, "uScreenSize");
    m_nodeUniforms.m_cameraPos = glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraPos");
    m_nodeUniforms.m_cameraZoom = glGetUniformLocation(nodeGL.m_shaderProgram, "uCameraZoom");
    m_nodeUniforms.m_nodeThickness =
        glGetUniformLocation(nodeGL.m_shaderProgram, "uOutlineThickness");
    m_nodeUniforms.m_selfLoops = glGetUniformLocation(nodeGL.m_shaderProgram, "uSelfLoops");

#ifdef __EMSCRIPTEN__
    m_nodeUniforms.m_textureWidth = glGetUniformLocation(nodeGL.m_shaderProgram, "uTextureWidth");
#endif
}

void GraphRenderer::initializeNodeFastGL() {
#ifndef __EMSCRIPTEN__
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
        uniform samplerBuffer uNodePositions;
        uniform usamplerBuffer uNodeColors;
        uniform usamplerBuffer uSelfLoops;

        uniform float uNodeRadius;
        uniform vec2 uScreenSize;
        uniform vec2 uCameraPos;
        uniform float uCameraZoom;

        out vec4 vColor;
        out vec4 vOutlineColor;
        flat out uint vHasSelfLoop;

        vec4 unpackColor(uint packedColor) {
            return vec4(
                float(packedColor & 0xFFu) / 255.0,
                float((packedColor >> 8) & 0xFFu) / 255.0,
                float((packedColor >> 16) & 0xFFu) / 255.0,
                float((packedColor >> 24) & 0xFFu) / 255.0
            );
        }

        uint hasSelfLoop(int lookupIndex) {
            uint indexInVector = uint(lookupIndex) / 8u;
            uint bitOffset = uint(lookupIndex) % 8u;

            return (texelFetch(uSelfLoops, int(indexInVector)).r >> bitOffset) & 1u;
        }

        void main() {
            vec2 worldPos = texelFetch(uNodePositions, int(gl_InstanceID)).xy;
            
            uvec4 colorsPacked = texelFetch(uNodeColors, int(gl_InstanceID));
            uint color = colorsPacked.x;
            uint outlineColor = colorsPacked.y;

            vec2 screenPos = (worldPos - uCameraPos) * uCameraZoom + uScreenSize * 0.5;
            vec2 ndc = (screenPos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);
            gl_PointSize = uNodeRadius * 2.0;

            vColor = unpackColor(color);
            vOutlineColor = unpackColor(outlineColor);
            vHasSelfLoop = hasSelfLoop(int(gl_InstanceID));
        }
)";

    constexpr auto fragmentShaderSource = GLSL_VERSION R"(
        in vec4 vColor;
        in vec4 vOutlineColor;
        flat in uint vHasSelfLoop;

        out vec4 FragColor;

        uniform float uOutlineThickness;

        void main() {
            vec2 d = gl_PointCoord - vec2(0.5);
            float dist2 = dot(d,d);

            const float r = 0.5;
            if (dist2 > r * r) {
                discard;
            }

            if (vHasSelfLoop == 1u) {
                const float innerR = 0.38;
                const float outerR = 0.4;
                
                if (dist2 > innerR * innerR && dist2 < outerR * outerR) {
                    FragColor = vOutlineColor;
                    return;
                }  
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
    m_fastNodeUniforms.m_selfLoops = glGetUniformLocation(nodeGL.m_shaderProgram, "uSelfLoops");
#endif
}

void GraphRenderer::initializeGridGL() {
    constexpr auto vertexShaderSource = GLSL_VERSION R"(
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aScreenStart;
        layout(location = 2) in vec2 aScreenEnd;
        layout(location = 3) in float aThickness;

        uniform vec2 uScreenSize;
        uniform uint uColor;

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
            vec2 lineDir = aScreenEnd - aScreenStart;
            vec2 normal = vec2(-lineDir.y, lineDir.x);
            if (length(normal) > 0.0) normal = normalize(normal);

            vec2 offset = normal * aThickness * (aPos.x - 0.5);
            vec2 pos = mix(aScreenStart, aScreenEnd, aPos.y) + offset;

            vec2 ndc = (pos / uScreenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);
            vColor = unpackColor(uColor);
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
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GridLineInstanceData),
                          (void*)(4 * sizeof(float)));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);

    m_gridUniformScreenSize = glGetUniformLocation(gridGL.m_shaderProgram, "uScreenSize");
    m_gridUniformColor = glGetUniformLocation(gridGL.m_shaderProgram, "uColor");
}

GLuint GraphRenderer::compileShader(GLenum type, const char* source) {
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

void GraphRenderer::setupNodeBuffers() {
    const auto& nodePositions = m_viewModel->getVisibleNodesPositions();
    const auto& nodeColors = m_viewModel->getVisibleNodesColors();
    const auto& selfLoops = m_viewModel->getVisibleLoops();

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

    if (m_selfLoopsDirty) {
#ifdef __EMSCRIPTEN__
        glBindTexture(GL_TEXTURE_2D, m_selfLoopsTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, m_maxTextureSize, textureHeight, 0, GL_RED_INTEGER,
                     GL_UNSIGNED_BYTE, selfLoops.data());
#else
        glBindBuffer(GL_TEXTURE_BUFFER, m_selfLoopsTBO);
        glBufferData(GL_TEXTURE_BUFFER, selfLoops.size() * sizeof(uint8_t), selfLoops.data(),
                     GL_DYNAMIC_DRAW);
#endif

        m_selfLoopsDirty = false;
    }
}

void GraphRenderer::drawBackground() {
    const auto backgroundColor =
        ImGui::ColorConvertU32ToFloat4(m_viewSettings->m_theme.m_backgroundColor);
    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
}

void GraphRenderer::drawGrid() {
    if (!m_viewSettings->m_drawGrid) {
        return;
    }

    std::vector<GridLineInstanceData> gridLines;

    const auto& io = ImGui::GetIO();
    const auto displaySize = io.DisplaySize;

    const auto gridCellSize = m_viewSettings->m_gridCellSize;
    const auto screenExtraPadding = Vector2D{gridCellSize, gridCellSize};
    const auto worldBounds = m_viewModel->getVisibleRegionWorld(screenExtraPadding);

    const auto topLeftWorld = worldBounds.m_min;
    const auto bottomRightWorld = worldBounds.m_max;

    const auto topLeftScreen = m_viewModel->worldToScreen(topLeftWorld);
    const auto bottomRightScreen = m_viewModel->worldToScreen(bottomRightWorld);

    const auto firstVerticalLineX =
        std::floor(topLeftWorld.m_x / gridCellSize) * gridCellSize + gridCellSize;
    for (float x = firstVerticalLineX; x < bottomRightWorld.m_x; x += gridCellSize) {
        const auto lineScreenPos = m_viewModel->worldToScreen({x, 0.f});
        const auto thickness = (std::abs(x) < 0.1f) ? 3.5f : 1.f;

        gridLines.emplace_back(Vector2D{lineScreenPos.m_x, topLeftScreen.m_y},
                               Vector2D{lineScreenPos.m_x, bottomRightScreen.m_y}, thickness);
    }

    const auto firstHorizontalLineY =
        std::floor(topLeftWorld.m_y / gridCellSize) * gridCellSize + gridCellSize;
    for (float y = firstHorizontalLineY; y < bottomRightWorld.m_y; y += gridCellSize) {
        const auto lineScreenPos = m_viewModel->worldToScreen({0.f, y});
        const auto thickness = (std::abs(y) < 0.1f) ? 3.5f : 1.f;

        gridLines.emplace_back(Vector2D{topLeftScreen.m_x, lineScreenPos.m_y},
                               Vector2D{bottomRightScreen.m_x, lineScreenPos.m_y}, thickness);
    }

    const auto& gridGL = m_gridGLObject;

    glBindVertexArray(gridGL.m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridGL.m_VBO);
    glBufferData(GL_ARRAY_BUFFER, gridLines.size() * sizeof(GridLineInstanceData), gridLines.data(),
                 GL_DYNAMIC_DRAW);

    glUseProgram(gridGL.m_shaderProgram);
    glUniform2f(m_gridUniformScreenSize, displaySize.x, displaySize.y);
    glUniform1ui(m_gridUniformColor, m_viewSettings->m_theme.m_gridColor);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)gridLines.size());

    glBindVertexArray(0);
}

void GraphRenderer::drawEdges() {
    if (!m_viewSettings->m_drawEdges) {
        return;
    }

    const auto [width, height] = ImGui::GetIO().DisplaySize;
    const auto zoom = m_viewModel->getZoomFactor();
    const auto [cameraX, cameraY] = m_viewModel->getCameraPosition();
    const auto shouldDrawArrowHeads = shouldDrawNodes();

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

    if (shouldDrawArrowHeads) {
        glUniform1f(m_edgeUniforms.m_nodeRadius, m_viewModel->getNodesRadius());
    } else {
        glUniform1f(m_edgeUniforms.m_nodeRadius, 0.1f);
    }

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

    if (shouldDrawArrowHeads) {
        glDrawArraysInstanced(GL_TRIANGLES, 2, 3, m_lastVisibleEdgesCount);
        glDrawArraysInstanced(GL_TRIANGLES, 5, 3, m_lastVisibleEdgesCount);
    }

    glBindVertexArray(0);
}

void GraphRenderer::drawNodes() {
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
    const auto shouldDrawFast = 2.f * radius < m_pointSizeRange[1] && false;
#endif

    const auto& nodeGL = shouldDrawFast ? m_nodeFastGLObject : m_nodeGLObject;
    const auto& uniforms = shouldDrawFast ? m_fastNodeUniforms : m_nodeUniforms;

    glBindVertexArray(nodeGL.m_VAO);

    glUseProgram(nodeGL.m_shaderProgram);
    glUniform1i(uniforms.m_nodePosition, 0);
    glUniform1i(uniforms.m_nodeColor, 1);
    glUniform1i(uniforms.m_selfLoops, 2);

    glUniform1f(uniforms.m_nodeRadius, radius);
    glUniform2f(uniforms.m_screenSize, width, height);
    glUniform2f(uniforms.m_cameraPos, cameraX, cameraY);
    glUniform1f(uniforms.m_cameraZoom, zoom);
    glUniform1f(uniforms.m_nodeThickness, m_viewSettings->m_drawNodesOutline
                                              ? (m_viewSettings->m_outlineThickness / 100.f)
                                              : 0.f);

#ifdef __EMSCRIPTEN__
    glUniform1i(m_nodeUniforms.m_textureWidth, m_maxTextureSize);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_nodePositionTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_nodeColorTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_selfLoopsTex);
#else
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_nodePositionTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, m_nodeColorTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, m_selfLoopsTex);
#endif

    const auto visibleNodesCount = static_cast<int>(visibleNodes.size());
    if (shouldDrawFast) {
        glDrawArraysInstanced(GL_POINTS, 0, 1, visibleNodesCount);
    } else {
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, visibleNodesCount);
    }

    glBindVertexArray(0);
}

void GraphRenderer::drawCosts(ImDrawList* drawList) {
    if (!shouldDrawNodes()) {
        return;
    }

    const auto zoom = m_viewModel->getZoomFactor();
    if (zoom < 0.72f) {
        return;
    }

    const auto& visibleEdges = m_viewModel->getVisibleEdges();
    const auto& nodeColors = m_viewModel->getVisibleNodesColors();
    const auto& visibleNodesIndexes = m_viewModel->getVisibleNodes();

    const auto drawEdgeWeight = [this, drawList, &nodeColors](NodeIndex_t src, NodeIndex_t dest,
                                                              uint32_t outlineColor) {
        const auto font = ImGui::GetIO().Fonts->Fonts[m_viewSettings->m_graphTextFontIndex];

        auto weight = m_model->getEdgeWeight(src, dest);
        if (weight == 0) {
            return;
        }

        const auto negative = weight < 0;

        char weightLabel[12];
        int len = 0;
        do {
            weightLabel[len++] = '0' + (weight % 10);
            weight /= 10;
        } while (weight > 0 && len < static_cast<int>(sizeof(weightLabel) - 1));

        if (negative) {
            weightLabel[len++] = '-';
        }

        weightLabel[len] = '\0';
        std::reverse(weightLabel, weightLabel + len);

        const auto srcNode = m_model->getNode(src);
        const auto destNode = m_model->getNode(dest);
        const auto srcScreenPos = m_viewModel->worldToScreen(srcNode->getWorldPos());
        const auto destScreenPos = m_viewModel->worldToScreen(destNode->getWorldPos());

        auto dir = destScreenPos - srcScreenPos;
        const auto lineLen = std::sqrt(dir.m_x * dir.m_x + dir.m_y * dir.m_y);
        if (lineLen > 0.01f) {
            dir = dir * (1.f / lineLen);
        }

        const auto perp = ImVec2{-dir.m_y, dir.m_x};
        const auto offset = perp * 15.f;
        const auto midPoint = toImVec((srcScreenPos + destScreenPos) * 0.5f) + offset;

        drawList->AddText(font, font->FontSize, midPoint, outlineColor, weightLabel);
    };

    for (auto [srcLookup, destLookupAndBothWays] : visibleEdges) {
        const auto isBothWays = (destLookupAndBothWays >> 31) & 1u;
        const auto destLookup = destLookupAndBothWays & 0x7FFFFFFF;

        const auto srcNodeIndex = visibleNodesIndexes[srcLookup];
        const auto destNodeIndex = visibleNodesIndexes[destLookup];

        drawEdgeWeight(srcNodeIndex, destNodeIndex, nodeColors[destLookup].m_outlineColor);
        if (isBothWays) {
            drawEdgeWeight(destNodeIndex, srcNodeIndex, nodeColors[srcLookup].m_outlineColor);
        }
    }
}

void GraphRenderer::drawNodesIndexes(ImDrawList* drawList) {
    if (!shouldDrawNodes()) {
        return;
    }

    const auto zoom = m_viewModel->getZoomFactor();
    if (zoom < 0.72f) {
        return;
    }

    const auto font = ImGui::GetIO().Fonts->Fonts[m_viewSettings->m_graphTextFontIndex];
    const auto& visibleNodesIndexes = m_viewModel->getVisibleNodes();
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

void GraphRenderer::drawMinMax(ImDrawList* drawList) {
    // The HARD limit of the world coordinates, we can draw it to visualize the limits of the
    // graph. This gets drawn regardless of the visible region, because it's useful to see it as
    // a reference when zooming out.
    const auto topLeftBoundsScreen = toImVec(m_viewModel->worldToScreen(WORLD_BOUNDS.m_min));
    const auto bottomRightBoundsScreen = toImVec(m_viewModel->worldToScreen(WORLD_BOUNDS.m_max));

    const auto& theme = m_viewSettings->m_theme;

    drawList->AddRect(topLeftBoundsScreen, bottomRightBoundsScreen, theme.m_gridColor, 0.f, 0, 3.f);

    if (!m_viewSettings->m_drawMinMax) {
        return;
    }

    const auto& bounds = m_model->getGraphBounds();

    const auto topLeftScreen = toImVec(m_viewModel->worldToScreen(bounds.m_min));
    const auto bottomRightScreen = toImVec(m_viewModel->worldToScreen(bounds.m_max));

    drawList->AddRect(topLeftScreen, bottomRightScreen, theme.m_minMaxColor, 0.f, 0, 1.f);
}

void GraphRenderer::drawSelectBox(ImDrawList* drawList) {
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

void GraphRenderer::drawMousePosition(ImDrawList* drawList) {
    if (isFocusOnUI() || m_viewModel->getHoveredNodeIndex() != INVALID_NODE) {
        return;
    }

    const auto [mouseX, mouseY] = ImGui::GetIO().MousePos;
    const auto mouseWorldPos = m_viewModel->screenToWorld({mouseX, mouseY});

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "(%.1f, %.1f)", mouseWorldPos.m_x, mouseWorldPos.m_y);

    const auto font = ImGui::GetIO().Fonts->Fonts[m_viewSettings->m_graphTextFontIndex];

    drawList->AddText(font, font->FontSize, {mouseX + 10.f, mouseY - 10.f},
                      m_viewSettings->m_theme.m_nodeOutlineColor, buffer);
}

bool GraphRenderer::shouldDrawNodes() const {
    return m_viewModel->getZoomFactor() > (m_viewSettings->m_nodeCutoffZoom / 100.f) &&
           m_viewSettings->m_drawNodes;
}

bool GraphRenderer::isFocusOnUI() const {
    return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

void GraphRenderer::colorNodes() {
    auto& nodeColors = m_viewModel->getVisibleNodesColors();

    for (uint32_t lookupIndex = 0; lookupIndex < nodeColors.size(); ++lookupIndex) {
        const auto nodeIndex = m_viewModel->getVisibleNodes()[lookupIndex];

        auto& nodeColor = nodeColors[lookupIndex];
        nodeColor.m_color = getNodeColor(nodeIndex);
        nodeColor.m_outlineColor = getOutlineColor(nodeIndex);
    }

    m_nodesColorDirty = true;
}

void GraphRenderer::colorNode(NodeIndex_t nodeIndex) {
    const auto lookupIndexOpt = m_viewModel->getLookupIndex(nodeIndex);
    if (!lookupIndexOpt.has_value()) {
        return;
    }

    const auto lookupIndex = lookupIndexOpt.value();
    auto& nodeColor = m_viewModel->getVisibleNodesColors()[lookupIndex];
    nodeColor.m_color = getNodeColor(nodeIndex);
    nodeColor.m_outlineColor = getOutlineColor(nodeIndex);

    m_nodesColorDirty = true;
}

ImU32 GraphRenderer::getNodeColor(NodeIndex_t nodeIndex) const {
    const auto node = m_model->getNode(nodeIndex);

    const auto& theme = m_viewSettings->m_theme;

    int nodeAlpha = (theme.m_nodeColor >> 24) & 0xFF;
    if (nodeIndex == m_viewModel->getHoveredNodeIndex()) {
        nodeAlpha = std::max(nodeAlpha - 60, 30);
    }

    if (node->hasCustomColor()) {
        return node->getABGR(nodeAlpha);
    }

    return theme.m_nodeColor & 0x00FFFFFF | (nodeAlpha << 24);
}

ImU32 GraphRenderer::getOutlineColor(NodeIndex_t nodeIndex) const {
    const auto isHovered = nodeIndex == m_viewModel->getHoveredNodeIndex();
    const auto isSelected = m_viewModel->isNodeSelected(nodeIndex);

    const auto& theme = m_viewSettings->m_theme;

    ImU32 color = theme.m_nodeOutlineColor;
    if (isSelected && isHovered) {
        color = theme.m_hoveredAndSelectedNodeOutlineColor;
    } else if (isHovered) {
        color = theme.m_hoveredNodeOutlineColor;
    } else if (isSelected) {
        color = theme.m_selectedNodeOutlineColor;
    }

    int outlineAlpha = (color >> 24) & 0xFF;
    if (isHovered) {
        outlineAlpha = std::max(outlineAlpha - 60, 30);
    }

    return color | (outlineAlpha << 24);
}
