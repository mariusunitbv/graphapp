#pragma once

#include "../GraphManager.h"
#include "../form/loading_screen/LoadingScreen.h"

class PBFLoader {
   public:
    PBFLoader(GraphManager* graphManager, const QString& pbfFile);

    void tryLoad();

   private:
    QPoint mercatorToGraphPosition(const QPointF& mercatorPos) const;

    void checkIfNodeForWaysNeeded();
    void parseAndComputeBounds();
    void addNodesToGraph();
    void connectNodes();

    bool shouldSkipHighway(const std::string_view highwayKey) const;

    struct WayData {
        std::vector<osmium::Location> m_locations{};
        bool m_oneWay{};
    };

    GraphManager* m_graphManager;
    LoadingScreen* m_loadingScreen{};

    std::string m_pbfPath;

    std::vector<WayData> m_ways{};
    QHash<QPoint, NodeIndex_t> m_screenToNodes{};

    qreal m_minX{std::numeric_limits<qreal>::max()};
    qreal m_maxX{std::numeric_limits<qreal>::min()};
    qreal m_minY{std::numeric_limits<qreal>::max()};
    qreal m_maxY{std::numeric_limits<qreal>::min()};

    float m_accuracy{};
    const osmium::geom::MercatorProjection m_projection{};
    bool m_shouldParseBoundaries : 1 {false};
    bool m_nodeForWaysNeeded : 1 {false};

    bool m_parseMotorways : 1 {false};
    bool m_parseMotorwayLinks : 1 {false};

    bool m_parseTrunks : 1 {false};
    bool m_parseTrunkLinks : 1 {false};

    bool m_parsePrimarys : 1 {false};
    bool m_parsePrimaryLinks : 1 {false};

    bool m_parseSecondarys : 1 {false};
    bool m_parseSecondaryLinks : 1 {false};

    bool m_parseTertiarys : 1 {false};
    bool m_parseTertiaryLinks : 1 {false};

    bool m_parseUnclassifieds : 1 {false};
    bool m_parseResidentials : 1 {false};
    bool m_parseLivingStreets : 1 {false};
    bool m_parseServices : 1 {false};
    bool m_parsePedestrians : 1 {false};
};
