module;
#include <pch.h>

export module graph_view_settings;

struct GraphTheme {
    ImU32 m_backgroundColor{IM_COL32(20, 20, 20, 255)};
    ImU32 m_gridColor{IM_COL32(35, 35, 35, 255)};
    ImU32 m_minMaxColor{IM_COL32(255, 0, 0, 255)};

    ImU32 m_nodeColor{IM_COL32(35, 35, 35, 255)};
    ImU32 m_nodeOutlineColor{IM_COL32(255, 255, 255, 255)};
    ImU32 m_selectedNodeOutlineColor{IM_COL32(89, 222, 18, 255)};
    ImU32 m_hoveredNodeOutlineColor{IM_COL32(18, 191, 222, 255)};
    ImU32 m_hoveredAndSelectedNodeOutlineColor{IM_COL32(18, 222, 130, 255)};
};

export struct GraphViewSettings {
    GraphTheme m_theme{};
};
