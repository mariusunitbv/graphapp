module;
#include <pch.h>

module gridmap;

import graph_model;

static constexpr auto CELL_SIZE = 1000;

void GridMap::insert(const Node* node) {
    const auto nodeArea = GraphModel::getNodeBoundingBox(node->m_worldPos);

    const auto [minCellX, maxCellX, minCellY, maxCellY] = calculateCellEntryForNode(nodeArea);
    for (size_t y = minCellY; y <= maxCellY; ++y) {
        for (size_t x = minCellX; x <= maxCellX; ++x) {
            m_cells[y * m_cellCountX + x].push_back(node->m_index);
        }
    }
}

void GridMap::remove(const std::vector<NodeIndex_t>& indexRemap) {
    for (auto& cellNodes : m_cells) {
        size_t i = 0;

        while (i < cellNodes.size()) {
            auto nodeIndex = cellNodes[i];

            if (indexRemap[nodeIndex] == INVALID_NODE) {
                cellNodes[i] = cellNodes.back();
                cellNodes.pop_back();
            } else {
                cellNodes[i] = indexRemap[nodeIndex];
                ++i;
            }
        }
    }
}

void GridMap::incrementNodeCountInCells(const BoundingBox2D& nodeArea) {
    if (m_nodeCountInCell.empty()) {
        m_nodeCountInCell.resize(m_cellCountX * m_cellCountY, 0);
    }

    const auto [minCellX, maxCellX, minCellY, maxCellY] = calculateCellEntryForNode(nodeArea);
    for (size_t y = minCellY; y <= maxCellY; ++y) {
        for (size_t x = minCellX; x <= maxCellX; ++x) {
            ++m_nodeCountInCell[y * m_cellCountX + x];
        }
    }
}

void GridMap::reserveNodeCountInCells() {
    if (m_nodeCountInCell.empty()) {
        GAPP_THROW("Node count in cells must be initialized before reserving node counts");
    }

    if (m_cells.size() != m_nodeCountInCell.size()) {
        GAPP_THROW("Cell count and node count in cells must be the same to reserve node counts");
    }

    for (size_t i = 0; i < m_nodeCountInCell.size(); ++i) {
        m_cells[i].reserve(m_nodeCountInCell[i]);
    }

    m_nodeCountInCell.clear();
    m_nodeCountInCell.shrink_to_fit();
}

std::vector<VisibleNode> GridMap::query(std::span<const Node> nodes,
                                        const BoundingBox2D& area) const {
    std::vector<VisibleNode> result;
    result.reserve(estimateNodeCountInArea(area));

    const auto [minCellX, maxCellX, minCellY, maxCellY] = calculateCellEntryForArea(area);
    for (size_t y = minCellY; y <= maxCellY; ++y) {
        for (size_t x = minCellX; x <= maxCellX; ++x) {
            const auto& cellNodes = m_cells[y * m_cellCountX + x];
            for (const auto nodeIndex : cellNodes) {
                const auto& node = nodes[nodeIndex];
                if (area.intersects(GraphModel::getNodeBoundingBox(node.m_worldPos))) {
                    result.emplace_back(node.m_worldPos, nodeIndex);
                }
            }
        }
    }

    return result;
}

NodeIndex_t GridMap::querySingle(std::span<const Node> nodes, Vector2D point, float minimumDistance,
                                 NodeIndex_t nodeToIgnore) const {
    auto bestDistanceSquared = minimumDistance * minimumDistance;

    const auto [minCellX, maxCellX, minCellY, maxCellY] =
        calculateCellEntryForArea(GraphModel::getNodeBoundingBox(point));

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
    const auto [minCellX, maxCellX, minCellY, maxCellY] = calculateCellEntryForArea(area);
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

bool GridMap::isAllocated() const { return m_cellCountX > 0 && m_cellCountY > 0; }

void GridMap::allocateCells() {
    const auto width = static_cast<int>(m_bounds.width());
    const auto height = static_cast<int>(m_bounds.height());

    m_cellCountX = (width + CELL_SIZE - 1) / CELL_SIZE;
    m_cellCountY = (height + CELL_SIZE - 1) / CELL_SIZE;

    m_cells.clear();
    m_cells.resize(m_cellCountX * m_cellCountY);
}

GridMap::EntryCell GridMap::calculateCellEntryForNode(const BoundingBox2D& nodeArea) const {
    if (m_cells.empty()) {
        return {1, 0, 1, 0};
    }

    const float centerX = (nodeArea.m_min.m_x + nodeArea.m_max.m_x) * 0.5f;
    const float centerY = (nodeArea.m_min.m_y + nodeArea.m_max.m_y) * 0.5f;

    const auto cellX = static_cast<int>(std::floor((centerX - m_bounds.m_min.m_x) / CELL_SIZE));
    const auto cellY = static_cast<int>(std::floor((centerY - m_bounds.m_min.m_y) / CELL_SIZE));

    EntryCell cell;
    cell.m_minCellX = cell.m_maxCellX = std::clamp(cellX, 0, m_cellCountX - 1);
    cell.m_minCellY = cell.m_maxCellY = std::clamp(cellY, 0, m_cellCountY - 1);

    return cell;
}

GridMap::EntryCell GridMap::calculateCellEntryForArea(const BoundingBox2D& nodeArea) const {
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

size_t GridMap::estimateNodeCountInArea(const BoundingBox2D& area) const {
    size_t estimatedCount = 0;

    const auto [minCellX, maxCellX, minCellY, maxCellY] = calculateCellEntryForArea(area);
    for (size_t y = minCellY; y <= maxCellY; ++y) {
        for (size_t x = minCellX; x <= maxCellX; ++x) {
            estimatedCount += m_cells[y * m_cellCountX + x].size();
        }
    }

    return estimatedCount;
}
