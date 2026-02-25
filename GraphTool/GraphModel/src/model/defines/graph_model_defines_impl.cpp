module;
#include <pch.h>

module graph_model_defines;

Node::Node(NodeIndex_t index, Vector2D worldPos) : m_index{index}, m_worldPos{worldPos} {
    m_red = (int)rand() % 211;
    m_green = (int)rand() % 211;
    m_blue = (int)rand() % 211;
}

bool Node::hasCustomColor() const { return !(m_red == 0 && m_green == 0 && m_blue == 0); }

uint32_t Node::getABGR() const {
    return static_cast<uint32_t>(255) << 24 | (static_cast<uint32_t>(m_blue) << 16) |
           (static_cast<uint32_t>(m_green) << 8) | static_cast<uint32_t>(m_red);
}
