module;
#include <pch.h>

module quadtree;

import graph_model;

static constexpr auto SOFT_MAX_NODES_PER_QUAD = 8;

QuadTree::QuadTree() { m_nodes.reserve(SOFT_MAX_NODES_PER_QUAD); }

void QuadTree::insert(const Node* node) {
    const auto nodeArea = GraphModel::getNodeBoundingBox(node->m_worldPos);
    if (!nodeArea.intersects(m_bounds)) {
        return;
    }

    if (m_nodes.size() < SOFT_MAX_NODES_PER_QUAD) {
        m_nodes.push_back(node->m_index);
        return;
    } else {
        if (!isSubdivided() && !canSubdivide()) {
            m_nodes.push_back(node->m_index);
            return;
        }
    }

    if (!isSubdivided()) {
        subdivide();
    }

    m_topLeft->insert(node);
    m_topRight->insert(node);
    m_bottomLeft->insert(node);
    m_bottomRight->insert(node);
}

void QuadTree::remove(NodeIndex_t toRemoveIndex, const BoundingBox2D& nodeArea) {
    if (!nodeArea.intersects(m_bounds)) {
        return;
    }

    for (auto& nodeIndex : m_nodes) {
        if (nodeIndex == toRemoveIndex) {
            nodeIndex = m_nodes.back();
            m_nodes.pop_back();
            break;
        }
    }

    if (isSubdivided()) {
        m_topLeft->remove(toRemoveIndex, nodeArea);
        m_topRight->remove(toRemoveIndex, nodeArea);
        m_bottomLeft->remove(toRemoveIndex, nodeArea);
        m_bottomRight->remove(toRemoveIndex, nodeArea);
    }
}

void QuadTree::clear() {
    m_nodes.clear();

    m_topLeft.reset();
    m_topRight.reset();
    m_bottomLeft.reset();
    m_bottomRight.reset();
}

void QuadTree::fixIndexesAfterNodeRemoval(const std::vector<NodeIndex_t>& indexRemap) {
    for (auto& nodeIndex : m_nodes) {
        nodeIndex = indexRemap[nodeIndex];
    }

    if (isSubdivided()) {
        m_topLeft->fixIndexesAfterNodeRemoval(indexRemap);
        m_topRight->fixIndexesAfterNodeRemoval(indexRemap);
        m_bottomLeft->fixIndexesAfterNodeRemoval(indexRemap);
        m_bottomRight->fixIndexesAfterNodeRemoval(indexRemap);
    }
}

std::vector<VisibleNode> QuadTree::query(std::span<const Node> nodes, const BoundingBox2D& area,
                                         size_t totalNodeCount) const {
    std::vector<bool> visitMask(totalNodeCount, false);
    std::vector<VisibleNode> result;

    query(nodes, area, visitMask, result);

    return result;
}

NodeIndex_t QuadTree::querySingle(std::span<const Node> nodes, const Vector2D& point,
                                  float minimumDistance, NodeIndex_t nodeToIgnore) const {
    NodeIndex_t closestNodeIndex = INVALID_NODE;

    auto minimumDistanceSquared = minimumDistance * minimumDistance;
    querySingle(nodes, point, GraphModel::getNodeBoundingBox(point), minimumDistanceSquared,
                nodeToIgnore, closestNodeIndex);

    return closestNodeIndex;
}

bool QuadTree::isSubdivided() const { return m_topLeft != nullptr; }

bool QuadTree::canSubdivide() const {
    return m_bounds.width() * 0.5f > 2 * NODE_RADIUS && m_bounds.height() * 0.5f > 2 * NODE_RADIUS;
}

void QuadTree::subdivide() {
    const auto [x, y] = m_bounds.m_min;
    const auto width = m_bounds.width() * 0.5f;
    const auto height = m_bounds.height() * 0.5f;

    if (!canSubdivide()) {
        GAPP_THROW("Cannot subdivide quad tree node with bounds: " + std::to_string(x) + ", " +
                   std::to_string(y) + ", " + std::to_string(m_bounds.m_max.m_x) + ", " +
                   std::to_string(m_bounds.m_max.m_y));
    }

    m_topLeft = std::make_unique<QuadTree>();
    m_topLeft->setBounds({x, y, x + width, y + height});

    m_topRight = std::make_unique<QuadTree>();
    m_topRight->setBounds({x + width, y, x + m_bounds.width(), y + height});

    m_bottomLeft = std::make_unique<QuadTree>();
    m_bottomLeft->setBounds({x, y + height, x + width, y + m_bounds.height()});

    m_bottomRight = std::make_unique<QuadTree>();
    m_bottomRight->setBounds({x + width, y + height, x + m_bounds.width(), y + m_bounds.height()});
}

const BoundingBox2D& QuadTree::getBounds() const { return m_bounds; }

BoundingBox2D& QuadTree::getBoundsMutable() { return m_bounds; }

void QuadTree::setBounds(const BoundingBox2D& bounds) { m_bounds = bounds; }

bool QuadTree::validBounds() const {
    return m_bounds.m_min.m_x < m_bounds.m_max.m_x && m_bounds.m_min.m_y < m_bounds.m_max.m_y;
}

void QuadTree::query(std::span<const Node> nodes, const BoundingBox2D& area,
                     std::vector<bool>& visitMask, std::vector<VisibleNode>& result) const {
    if (!area.intersects(m_bounds)) {
        return;
    }

    for (const auto nodeIndex : m_nodes) {
        if (visitMask[nodeIndex]) {
            continue;
        }

        const auto& node = nodes[nodeIndex];
        const auto nodePosition = node.m_worldPos;
        if (area.intersects(GraphModel::getNodeBoundingBox(nodePosition))) {
            visitMask[nodeIndex] = true;
            result.emplace_back(nodePosition, node.getRGB(), nodeIndex);
        }
    }

    if (isSubdivided()) {
        m_topLeft->query(nodes, area, visitMask, result);
        m_topRight->query(nodes, area, visitMask, result);
        m_bottomLeft->query(nodes, area, visitMask, result);
        m_bottomRight->query(nodes, area, visitMask, result);
    }
}

void QuadTree::querySingle(std::span<const Node> nodes, const Vector2D& point,
                           const BoundingBox2D& area, float& minimumDistanceSquared,
                           NodeIndex_t nodeToIgnore, NodeIndex_t& closestNodeIndex) const {
    if (!area.intersects(m_bounds)) {
        return;
    }

    for (const auto nodeIndex : m_nodes) {
        if (nodeIndex == nodeToIgnore || nodeIndex == closestNodeIndex) {
            continue;
        }

        const auto nodePosition = nodes[nodeIndex].m_worldPos;
        const auto distance = nodePosition.distanceSquared(point);
        if (distance < minimumDistanceSquared) {
            minimumDistanceSquared = distance;
            closestNodeIndex = nodeIndex;
        }
    }

    if (isSubdivided()) {
        m_topLeft->querySingle(nodes, point, area, minimumDistanceSquared, nodeToIgnore,
                               closestNodeIndex);
        m_topRight->querySingle(nodes, point, area, minimumDistanceSquared, nodeToIgnore,
                                closestNodeIndex);
        m_bottomLeft->querySingle(nodes, point, area, minimumDistanceSquared, nodeToIgnore,
                                  closestNodeIndex);
        m_bottomRight->querySingle(nodes, point, area, minimumDistanceSquared, nodeToIgnore,
                                   closestNodeIndex);
    }
}
