#pragma once

#include "../GraphManager.h"
#include "../form/loading_screen/LoadingScreen.h"

class PBFLoader {
   public:
    PBFLoader(GraphManager* graphManager, const QString& pbfFile);

    void tryLoad();

   private:
    QPoint mercatorToGraphPosition(const QPointF& mercatorPos) const;

    void parseAndComputeBounds();
    void addNodesToGraph();
    void connectNodes();

    struct WayData {
        std::vector<std::pair<QPointF, int64_t>> m_mercatorPoints{};
        bool m_oneWay{};
    };

    GraphManager* m_graphManager;
    LoadingScreen* m_loadingScreen{};

    std::string m_pbfPath;

    std::vector<WayData> m_ways{};
    QHash<QPoint, NodeIndex_t> m_screenToNodes{};
    float m_accuracy{};

    qreal m_minX{std::numeric_limits<qreal>::max()};
    qreal m_maxX{std::numeric_limits<qreal>::min()};
    qreal m_minY{std::numeric_limits<qreal>::max()};
    qreal m_maxY{std::numeric_limits<qreal>::min()};
};
