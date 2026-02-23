module;
#include <pch.h>

module graph_model_defines;

Node::Node(NodeIndex_t index, Vector2D worldPos) : m_worldPos{worldPos} {
    setIndex(index);

    m_red = (int)rand() % 211;
    m_green = (int)rand() % 211;
    m_blue = (int)rand() % 211;
}

void Node::setIndex(NodeIndex_t index) {
    m_index = index;

    size_t charCount = 1;
    while (index >= 10) {
        index /= 10;
        ++charCount;
    }

    if (charCount > sizeof(m_labelBuffer) - 1) {
        GAPP_THROW("Node index exceeds label buffer size");
    }

    std::snprintf(m_labelBuffer, charCount + 1, "%u", m_index);
}

bool Node::hasColor() const { return !(m_red == 0 && m_green == 0 && m_blue == 0); }

uint32_t Node::getABGR() const {
    return static_cast<uint32_t>(m_alpha) << 24 | (static_cast<uint32_t>(m_blue) << 16) |
           (static_cast<uint32_t>(m_green) << 8) | static_cast<uint32_t>(m_red);
}
