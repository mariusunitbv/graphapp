#pragma once

#include "../GraphManager.h"
#include "../form/loading_screen/LoadingScreen.h"

class GeoJSONLoader {
   public:
    GeoJSONLoader(GraphManager* graphManager, const QString& geoJsonFile);

    void tryLoad();

   private:
    QPointF toMercator(qreal lat, qreal lon) const;
    QPoint mercatorToGraphPosition(const QPointF& mercatorPos) const;

    void parseAndComputeBounds();
    void addNodesToGraph();
    void connectNodes();

    struct WayData {
        std::vector<QPointF> m_mercatorPoints{};
        bool m_oneWay{};
    };

    GraphManager* m_graphManager;
    LoadingScreen* m_loadingScreen{};

    std::string m_jsonPath;
    std::vector<WayData> m_ways{};

    float m_accuracy{};

    qreal m_minX{std::numeric_limits<qreal>::max()};
    qreal m_maxX{std::numeric_limits<qreal>::min()};
    qreal m_minY{std::numeric_limits<qreal>::max()};
    qreal m_maxY{std::numeric_limits<qreal>::min()};
};
