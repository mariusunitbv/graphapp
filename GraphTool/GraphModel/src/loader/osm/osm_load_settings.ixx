module;
#include <pch.h>

export module osm_load_settings;

export struct OSMLoadSettings {
    float m_worldBounds{30000.f};

    float m_mergeCloseNodesDistance{3.f};
    bool m_mergeCloseNodes{false};

    bool m_shouldParseBoundaries{false};
    bool m_shouldParseHighways{true};

    bool m_parseMotorways{true};
    bool m_parseMotorwayLinks{true};

    bool m_parseTrunks{true};
    bool m_parseTrunkLinks{true};

    bool m_parsePrimarys{true};
    bool m_parsePrimaryLinks{true};

    bool m_parseSecondarys{false};
    bool m_parseSecondaryLinks{false};

    bool m_parseTertiarys{false};
    bool m_parseTertiaryLinks{false};

    bool m_parseUnclassifieds{false};
    bool m_parseResidentials{false};
    bool m_parseLivingStreets{false};
    bool m_parseServices{false};
    bool m_parsePedestrians{false};

    bool m_shouldParseRailways{false};
    bool m_parseRails{true};
    bool m_parseLightRails{true};
    bool m_parseSubways{true};
    bool m_parseTrams{false};
};
