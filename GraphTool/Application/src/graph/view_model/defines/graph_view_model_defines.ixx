module;
#include <pch.h>

export module graph_view_model_defines;

import graph_model_defines;

export struct GraphCamera {
    Vector2D m_position{};
    float m_zoom{1.0f};
};

export struct VisibleNode {
    VisibleNode() = default;
    explicit VisibleNode(Vector2D worldPos, NodeIndex_t index)
        : m_worldPos{worldPos}, m_index{index} {}

    Vector2D m_worldPos;
    uint32_t m_color;
    uint32_t m_outlineColor;

    NodeIndex_t m_index;
};
