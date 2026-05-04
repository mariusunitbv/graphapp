module;
#include <pch.h>

export module haversine_heuristic;

import heuristic;

export class HaversineHeuristic : public IHeuristic {
   public:
    HaversineHeuristic(const std::unordered_map<std::string, std::string>& metadata);

    HeuristicType getType() const override;
    double distance(const Node* a, const Node* b) const override;

   private:
    Vector2D worldToMercator(Vector2D worldPos) const;

    static double radToDeg(double r);
    static double degToRad(double d);

    static double mercatorXToLon(double x);
    static double mercatorYToLat(double y);
    static double haversine(double lat1, double lon1, double lat2, double lon2);

    static constexpr double R = 6378137.0;
    static constexpr double PI = 3.14159265358979323846;

    float m_minX{0}, m_minY{0}, m_maxX{0}, m_maxY{0};
    float m_dataWidth{0}, m_dataHeight{0};
    float m_scaledWidth{0}, m_scaledHeight{0};
    float m_scaledPaddingX{0}, m_scaledPaddingY{0};
    float m_worldBounds{0};
};
