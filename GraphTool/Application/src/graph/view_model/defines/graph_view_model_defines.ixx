module;
#include <pch.h>

export module graph_view_model_defines;

import graph_model_defines;

export struct GraphCamera {
    Vector2D m_position{};
    float m_zoom{1.0f};
};

export struct NodeColorInfo {
    uint32_t m_color;
    uint32_t m_outlineColor;
};

export struct VisibleNode {
    uint32_t m_lookupIndex;
};

export struct VisibleEdge {
    uint32_t m_startNodeIndexLookup;
    uint32_t m_endNodeIndexLookup;
};

export struct VisibleData {
    std::vector<Vector2D> m_nodesPositions;
    std::vector<NodeColorInfo> m_nodesColors;
    std::vector<NodeIndex_t> m_nodesIndexes;

    std::vector<VisibleNode> m_visibleNodes;
    std::vector<VisibleEdge> m_visibleEdges;
};
