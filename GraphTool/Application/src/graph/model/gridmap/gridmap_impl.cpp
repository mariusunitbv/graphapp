module;
#include <pch.h>

module gridmap;

import graph_model;

static constexpr auto CELL_SIZE = 1000;

void GridMap::insert(const Node* node) {
    const auto nodeArea = GraphModel::getNodeBoundingBox(node->m_worldPos);

    const auto [minCellX, maxCellX, minCellY, maxCellY] = calculateEntryCell(nodeArea);
    for (size_t y = minCellY; y <= maxCellY; ++y) {
        for (size_t x = minCellX; x <= maxCellX; ++x) {
            m_cells[y * m_cellCountX + x].push_back(node->m_index);
        }
    }
}

void GridMap::remove(NodeIndex_t toRemoveIndex, const BoundingBox2D& nodeArea) {
    const auto [minCellX, maxCellX, minCellY, maxCellY] = calculateEntryCell(nodeArea);
    for (size_t y = minCellY; y <= maxCellY; ++y) {
        for (size_t x = minCellX; x <= maxCellX; ++x) {
            auto& cellNodes = m_cells[y * m_cellCountX + x];
            for (auto& nodeIndex : cellNodes) {
                if (nodeIndex == toRemoveIndex) {
                    nodeIndex = cellNodes.back();
                    cellNodes.pop_back();
                    break;
                }
            }
        }
    }
}

void GridMap::fixIndexesAfterNodeRemoval(const std::vector<NodeIndex_t>& indexRemap) {
    for (auto& cell : m_cells) {
        for (auto& nodeIndex : cell) {
            nodeIndex = indexRemap[nodeIndex];
        }
    }
}

std::vector<VisibleNode> GridMap::query(std::span<const Node> nodes,
                                        const BoundingBox2D& area) const {
    std::vector<bool> visitMask(nodes.size(), false);
    std::vector<VisibleNode> result;

    query(nodes, area, visitMask, result);

    return result;
}

NodeIndex_t GridMap::querySingle(std::span<const Node> nodes, Vector2D point, float minimumDistance,
                                 NodeIndex_t nodeToIgnore) const {
    auto bestDistanceSquared = minimumDistance * minimumDistance;

    const auto [minCellX, maxCellX, minCellY, maxCellY] =
        calculateEntryCell(GraphModel::getNodeBoundingBox(point));

    NodeIndex_t closestNodeIndex = INVALID_NODE;
    for (size_t y = minCellY; y <= maxCellY; ++y) {
        for (size_t x = minCellX; x <= maxCellX; ++x) {
            const auto& cellNodes = m_cells[y * m_cellCountX + x];
            for (const auto nodeIndex : cellNodes) {
                if (nodeIndex == nodeToIgnore) {
                    continue;
                }

                const auto& node = nodes[nodeIndex];
                const auto distanceSquared = node.m_worldPos.distanceSquared(point);
                if (distanceSquared < bestDistanceSquared) {
                    bestDistanceSquared = distanceSquared;
                    closestNodeIndex = nodeIndex;
                }
            }
        }
    }

    return closestNodeIndex;
}

NodeIndex_t GridMap::querySingleFast(std::span<const Node> nodes, Vector2D point,
                                     const BoundingBox2D& area, float minimumDistance,
                                     NodeIndex_t nodeToIgnore) const {
    const auto [minCellX, maxCellX, minCellY, maxCellY] = calculateEntryCell(area);
    for (size_t y = minCellY; y <= maxCellY; ++y) {
        for (size_t x = minCellX; x <= maxCellX; ++x) {
            const auto& cellNodes = m_cells[y * m_cellCountX + x];
            for (const auto nodeIndex : cellNodes) {
                if (nodeIndex == nodeToIgnore) {
                    continue;
                }

                const auto& node = nodes[nodeIndex];
                const auto distanceSquared = node.m_worldPos.distanceSquared(point);
                if (distanceSquared < minimumDistance * minimumDistance) {
                    return node.m_index;
                }
            }
        }
    }

    return INVALID_NODE;
}

const BoundingBox2D& GridMap::getBounds() const { return m_bounds; }

void GridMap::setBounds(const BoundingBox2D& bounds) { m_bounds = bounds; }

void GridMap::allocateCells() {
    const auto width = static_cast<int>(m_bounds.width());
    const auto height = static_cast<int>(m_bounds.height());

    m_cellCountX = (width + CELL_SIZE - 1) / CELL_SIZE;
    m_cellCountY = (height + CELL_SIZE - 1) / CELL_SIZE;

    m_cells.clear();
    m_cells.resize(m_cellCountX * m_cellCountY);
}

GridMap::EntryCell GridMap::calculateEntryCell(const BoundingBox2D& nodeArea) const {
    if (m_cells.empty()) {
        return {1, 0, 1, 0};
    }

    const float relMinX = nodeArea.m_min.m_x - m_bounds.m_min.m_x;
    const float relMaxX = nodeArea.m_max.m_x - m_bounds.m_min.m_x;
    const float relMinY = nodeArea.m_min.m_y - m_bounds.m_min.m_y;
    const float relMaxY = nodeArea.m_max.m_y - m_bounds.m_min.m_y;

    const auto minCellX = static_cast<int>(std::floor(relMinX / CELL_SIZE));
    const auto maxCellX = static_cast<int>(std::ceil(relMaxX / CELL_SIZE)) - 1;
    const auto minCellY = static_cast<int>(std::floor(relMinY / CELL_SIZE));
    const auto maxCellY = static_cast<int>(std::ceil(relMaxY / CELL_SIZE)) - 1;

    EntryCell cell;
    cell.m_minCellX = std::clamp(minCellX, 0, m_cellCountX - 1);
    cell.m_maxCellX = std::clamp(maxCellX, 0, m_cellCountX - 1);
    cell.m_minCellY = std::clamp(minCellY, 0, m_cellCountY - 1);
    cell.m_maxCellY = std::clamp(maxCellY, 0, m_cellCountY - 1);

    return cell;
}

void GridMap::query(std::span<const Node> nodes, const BoundingBox2D& area,
                    std::vector<bool>& visitMask, std::vector<VisibleNode>& result) const {
    const auto [minCellX, maxCellX, minCellY, maxCellY] = calculateEntryCell(area);
    for (size_t y = minCellY; y <= maxCellY; ++y) {
        for (size_t x = minCellX; x <= maxCellX; ++x) {
            const auto& cellNodes = m_cells[y * m_cellCountX + x];
            for (const auto nodeIndex : cellNodes) {
                if (visitMask[nodeIndex]) {
                    continue;
                }

                const auto& node = nodes[nodeIndex];
                if (area.contains(node.m_worldPos)) {
                    visitMask[nodeIndex] = true;
                    result.emplace_back(node.m_worldPos, node.getABGR(), nodeIndex);
                }
            }
        }
    }
}
