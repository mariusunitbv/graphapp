module;
#include <pch.h>

module quadtree;

import graph_view;

QuadTree::QuadTree() { m_nodes.reserve(SOFT_MAX_NODES_PER_QUAD); }

void QuadTree::insert(const Node* node) {
    const auto bbox = GraphView::getNodeWorldBBox(node);
    if (!intersects(bbox, m_bounds)) {
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

void QuadTree::remove(GraphModel* graphModel, const std::unordered_set<NodeIndex_t>& nodes) {
    for (const auto nodeIndex : nodes) {
        const auto nodeArea = GraphView::getNodeWorldBBox(&graphModel->getNodes()[nodeIndex]);
        remove(nodeIndex, nodeArea);
    }
}

void QuadTree::remove(NodeIndex_t toRemoveIndex, const ImVec4& nodeArea) {
    if (!intersects(m_bounds, nodeArea)) {
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

void QuadTree::fixIndexesAfterNodeRemoval(GraphModel* graphModel) {
    for (auto& nodeIndex : m_nodes) {
        nodeIndex = graphModel->getNodeIndexAfterRemoval(nodeIndex);
    }

    if (isSubdivided()) {
        m_topLeft->fixIndexesAfterNodeRemoval(graphModel);
        m_topRight->fixIndexesAfterNodeRemoval(graphModel);
        m_bottomLeft->fixIndexesAfterNodeRemoval(graphModel);
        m_bottomRight->fixIndexesAfterNodeRemoval(graphModel);
    }
}

std::vector<NodeIndex_t> QuadTree::query(GraphModel* graphModel, const ImVec4& area,
                                         size_t totalNodeCount) const {
    std::vector<bool> visitMask(totalNodeCount, false);
    std::vector<NodeIndex_t> result;

    query(graphModel, area, visitMask, result);

    return result;
}

NodeIndex_t QuadTree::querySingle(GraphModel* graphModel, const ImVec2& point,
                                  float minimumDistance, NodeIndex_t nodeToIgnore) const {
    NodeIndex_t closestNodeIndex = INVALID_NODE;

    auto minimumDistanceSquared = minimumDistance * minimumDistance;
    querySingle(graphModel, point, GraphView::getNodeWorldBBox(point), minimumDistanceSquared,
                nodeToIgnore, closestNodeIndex);

    return closestNodeIndex;
}

bool QuadTree::isSubdivided() const { return m_topLeft != nullptr; }

bool QuadTree::canSubdivide() const {
    const auto width = m_bounds.z - m_bounds.x;
    const auto height = m_bounds.w - m_bounds.y;

    return width * 0.5f > 2 * GraphView::NODE_RADIUS && height * 0.5f > 2 * GraphView::NODE_RADIUS;
}

void QuadTree::subdivide() {
    const auto x = m_bounds.x;
    const auto y = m_bounds.y;
    const auto width = (m_bounds.z - m_bounds.x) * 0.5f;
    const auto height = (m_bounds.w - m_bounds.y) * 0.5f;

    if (!canSubdivide()) {
        GAPP_THROW("Cannot subdivide quad tree node with bounds: " + std::to_string(m_bounds.x) +
                   ", " + std::to_string(m_bounds.y) + ", " + std::to_string(m_bounds.z) + ", " +
                   std::to_string(m_bounds.w));
    }

    m_topLeft = std::make_unique<QuadTree>();
    m_topLeft->setBounds(x, y, x + width, y + height);

    m_topRight = std::make_unique<QuadTree>();
    m_topRight->setBounds(x + width, y, x + 2.f * width, y + height);

    m_bottomLeft = std::make_unique<QuadTree>();
    m_bottomLeft->setBounds(x, y + height, x + width, y + 2.f * height);

    m_bottomRight = std::make_unique<QuadTree>();
    m_bottomRight->setBounds(x + width, y + height, x + 2.f * width, y + 2.f * height);
}

const ImVec4& QuadTree::getBounds() const { return m_bounds; }

void QuadTree::setBounds(float left, float top, float right, float bottom) {
    m_bounds = ImVec4{left, top, right, bottom};
}

bool QuadTree::validBounds() const { return m_bounds.x < m_bounds.z && m_bounds.y < m_bounds.w; }

void QuadTree::query(GraphModel* graphModel, const ImVec4& area, std::vector<bool>& visitMask,
                     std::vector<NodeIndex_t>& result) const {
    if (!intersects(m_bounds, area)) {
        return;
    }

    for (const auto nodeIndex : m_nodes) {
        if (visitMask[nodeIndex]) {
            continue;
        }

        const auto node = &graphModel->getNodes()[nodeIndex];
        if (intersects(area, GraphView::getNodeWorldBBox(node))) {
            visitMask[nodeIndex] = true;
            result.push_back(nodeIndex);
        }
    }

    if (isSubdivided()) {
        m_topLeft->query(graphModel, area, visitMask, result);
        m_topRight->query(graphModel, area, visitMask, result);
        m_bottomLeft->query(graphModel, area, visitMask, result);
        m_bottomRight->query(graphModel, area, visitMask, result);
    }
}

void QuadTree::querySingle(GraphModel* graphModel, const ImVec2& point, const ImVec4& area,
                           float& minimumDistanceSquared, NodeIndex_t nodeToIgnore,
                           NodeIndex_t& closestNodeIndex) const {
    if (!intersects(m_bounds, area)) {
        return;
    }

    for (const auto nodeIndex : m_nodes) {
        if (nodeIndex == nodeToIgnore || nodeIndex == closestNodeIndex) {
            continue;
        }

        const auto node = &graphModel->getNodes()[nodeIndex];
        const auto distance = distanceSquared({node->m_x, node->m_y}, point);
        if (distance < minimumDistanceSquared) {
            minimumDistanceSquared = distance;
            closestNodeIndex = nodeIndex;
        }
    }

    if (isSubdivided()) {
        m_topLeft->querySingle(graphModel, point, area, minimumDistanceSquared, nodeToIgnore,
                               closestNodeIndex);
        m_topRight->querySingle(graphModel, point, area, minimumDistanceSquared, nodeToIgnore,
                                closestNodeIndex);
        m_bottomLeft->querySingle(graphModel, point, area, minimumDistanceSquared, nodeToIgnore,
                                  closestNodeIndex);
        m_bottomRight->querySingle(graphModel, point, area, minimumDistanceSquared, nodeToIgnore,
                                   closestNodeIndex);
    }
}
