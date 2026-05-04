module;
#include <pch.h>

module haversine_heuristic;

HaversineHeuristic::HaversineHeuristic(
    const std::unordered_map<std::string, std::string>& metadata) {
    m_minX = std::stof(metadata.at("min_x"));
    m_minY = std::stof(metadata.at("min_y"));
    m_maxX = std::stof(metadata.at("max_x"));
    m_maxY = std::stof(metadata.at("max_y"));
    m_dataWidth = std::stof(metadata.at("data_width"));
    m_dataHeight = std::stof(metadata.at("data_height"));
    m_scaledWidth = std::stof(metadata.at("scaled_width"));
    m_scaledHeight = std::stof(metadata.at("scaled_height"));
    m_scaledPaddingX = std::stof(metadata.at("scaled_padding_x"));
    m_scaledPaddingY = std::stof(metadata.at("scaled_padding_y"));
    m_worldBounds = std::stof(metadata.at("world_bounds"));
}

HeuristicType HaversineHeuristic::getType() const { return HeuristicType::HAVERSINE; }

double HaversineHeuristic::distance(const Node* a, const Node* b) const {
    const auto p1 = worldToMercator(a->getWorldPos());
    const auto p2 = worldToMercator(b->getWorldPos());

    const double lat1 = mercatorYToLat(p1.m_y);
    const double lon1 = mercatorXToLon(p1.m_x);

    const double lat2 = mercatorYToLat(p2.m_y);
    const double lon2 = mercatorXToLon(p2.m_x);

    return haversine(lat1, lon1, lat2, lon2);
}

Vector2D HaversineHeuristic::worldToMercator(Vector2D worldPos) const {
    const auto nx = (worldPos.m_x + m_worldBounds - m_scaledPaddingX) / m_scaledWidth;
    const auto ny = 1.f - (worldPos.m_y + m_worldBounds - m_scaledPaddingY) / m_scaledHeight;

    const auto x = m_minX + nx * m_dataWidth;
    const auto y = m_minY + ny * m_dataHeight;

    return {x, y};
}

double HaversineHeuristic::radToDeg(double r) { return r * 180.0 / PI; }

double HaversineHeuristic::degToRad(double d) { return d * PI / 180.0; }

double HaversineHeuristic::mercatorXToLon(double x) { return radToDeg(x / R); }

double HaversineHeuristic::mercatorYToLat(double y) {
    return radToDeg(2.0 * std::atan(std::exp(y / R)) - PI / 2.0);
}

double HaversineHeuristic::haversine(double lat1, double lon1, double lat2, double lon2) {
    const double dLat = degToRad(lat2 - lat1);
    const double dLon = degToRad(lon2 - lon1);

    lat1 = degToRad(lat1);
    lat2 = degToRad(lat2);

    const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                     std::cos(lat1) * std::cos(lat2) * std::sin(dLon / 2) * std::sin(dLon / 2);

    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

    return R * c;
}
