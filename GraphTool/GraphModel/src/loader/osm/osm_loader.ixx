module;
#include <pch.h>

export module osm_loader;

import graph_model;

export class OSMLoader {
   public:
    OSMLoader(GraphModel* model, const std::string_view osmFile);

    void loadGraph();

#ifndef __EMSCRIPTEN__
   private:
    Vector2D mercatorToWorld(const Vector2D& mercatorPos) const;

    void getNeededNodes();
    void parseAndComputeBounds();
    void addNodesToGraph();

    GraphModel* m_model{nullptr};
    std::string m_osmPath;

    struct WayMeta {
        explicit WayMeta(uint32_t startIndex, bool isOneWay)
            : m_startIndex(startIndex), m_isOneWay(isOneWay) {}

        uint32_t m_startIndex : 31 {};
        uint32_t m_isOneWay : 1 {};
    };

    phmap::parallel_flat_hash_map<osmium::unsigned_object_id_type, osmium::Location>
        m_nodesLocations{};

    std::vector<osmium::Location> m_nodesForWays{};
    std::vector<WayMeta> m_waysMeta{};

    double m_minX{std::numeric_limits<double>::max()};
    double m_maxX{std::numeric_limits<double>::min()};
    double m_minY{std::numeric_limits<double>::max()};
    double m_maxY{std::numeric_limits<double>::min()};

    BoundingBox2D m_mapBounds{};

    float m_dataWidth{0};
    float m_dataHeight{0};

    float m_scaledWidth{0};
    float m_scaledHeight{0};
    float m_scaledPaddingX{0};
    float m_scaledPaddingY{0};

    uint32_t m_parsedWayCount{0};
    uint32_t m_parsedNodeCount{0};

    const osmium::geom::MercatorProjection m_projection{};

    bool m_nodeForWaysNeeded : 1 {true};
#endif
};
