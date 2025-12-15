#include <pch.h>

#include "QuadTree.h"

QuadTree::~QuadTree() {
    if (isSubdivided()) {
        delete m_northWest;
        delete m_northEast;
        delete m_southWest;
        delete m_southEast;
    }
}

void QuadTree::setBoundary(const QRect& boundary) { m_boundary = boundary; }

const QRect& QuadTree::getBoundary() const { return m_boundary; }

void QuadTree::insert(const NodeData& node) {
    if (!m_boundary.intersects(node.getBoundingRect())) {
        return;
    }

    if (m_nodes.size() < k_maxSoftCapacity) {
        m_nodes.emplace_back(node.getIndex(), node.getPosition());
        return;
    } else {
        if (!isSubdivided() && !canSubdivide()) {
            m_nodes.emplace_back(node.getIndex(), node.getPosition());
            return;
        }
    }

    if (!isSubdivided()) {
        subdivide();
    }

    m_northWest->insert(node);
    m_northEast->insert(node);
    m_southWest->insert(node);
    m_southEast->insert(node);
}

void QuadTree::getContainingTrees(const NodeData& node, std::vector<QuadTree*>& trees) {
    if (!m_boundary.intersects(node.getBoundingRect())) {
        return;
    }

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].m_index == node.getIndex()) {
            return trees.push_back(this);
        }
    }

    if (!isSubdivided()) {
        return;
    }

    m_northWest->getContainingTrees(node, trees);
    m_northEast->getContainingTrees(node, trees);
    m_southWest->getContainingTrees(node, trees);
    m_southEast->getContainingTrees(node, trees);
}

void QuadTree::getNodesInArea(const QRect& area, std::unordered_set<NodeIndex_t>& nodes) const {
    if (!m_boundary.intersects(area)) {
        return;
    }

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const auto& nodePos = m_nodes[i].m_position;
        const QRect nodeArea(nodePos.x() - NodeData::k_radius, nodePos.y() - NodeData::k_radius,
                             2 * NodeData::k_radius, 2 * NodeData::k_radius);

        if (area.intersects(nodeArea)) {
            nodes.emplace(m_nodes[i].m_index);
        }
    }

    if (!isSubdivided()) {
        return;
    }

    m_northWest->getNodesInArea(area, nodes);
    m_northEast->getNodesInArea(area, nodes);
    m_southWest->getNodesInArea(area, nodes);
    m_southEast->getNodesInArea(area, nodes);
}

bool QuadTree::needsReinserting(const NodeData& node) const {
    return !m_boundary.intersects(node.getBoundingRect());
}

void QuadTree::update(const NodeData& node) {
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].m_index == node.getIndex()) {
            m_nodes[i].m_position = node.getPosition();
            return;
        }
    }
}

void QuadTree::remove(const NodeData& node) {
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].m_index == node.getIndex()) {
            m_nodes[i] = m_nodes[m_nodes.size() - 1];
            m_nodes.pop_back();
            return;
        }
    }
}

void QuadTree::clear() {
    m_nodes.clear();
    if (isSubdivided()) {
        delete m_northWest;
        delete m_northEast;
        delete m_southWest;
        delete m_southEast;

        m_northWest = m_northEast = m_southWest = m_southEast = nullptr;
    }
}

void QuadTree::subdivide() {
    const auto x = m_boundary.x();
    const auto y = m_boundary.y();
    const auto w = m_boundary.width() / 2;
    const auto h = m_boundary.height() / 2;

    if (!canSubdivide()) {
        throw std::runtime_error("Cannot subdivide QuadTree further.");
    }

    m_northWest = new QuadTree();
    m_northWest->setBoundary(QRect(x, y, w, h));

    m_northEast = new QuadTree();
    m_northEast->setBoundary(QRect(x + w, y, w, h));

    m_southWest = new QuadTree();
    m_southWest->setBoundary(QRect(x, y + h, w, h));

    m_southEast = new QuadTree();
    m_southEast->setBoundary(QRect(x + w, y + h, w, h));
}

bool QuadTree::isSubdivided() const { return m_northWest; }

bool QuadTree::canSubdivide() const {
    return m_boundary.width() / 2 > 0 && m_boundary.height() / 2 > 0;
}

QuadTree* QuadTree::getNorthWest() const { return m_northWest; }

QuadTree* QuadTree::getNorthEast() const { return m_northEast; }

QuadTree* QuadTree::getSouthWest() const { return m_southWest; }

QuadTree* QuadTree::getSouthEast() const { return m_southEast; }

bool QuadTree::intersectsAnotherNode(QPoint pos, NodeIndex_t indexToIgnore) const {
    return getNodeAtPosition(pos, 2 * NodeData::k_radius, indexToIgnore).has_value();
}

std::optional<NodeIndex_t> QuadTree::getNodeAtPosition(QPoint pos, float minDistance,
                                                       NodeIndex_t indexToIgnore) const {
    const auto minDistanceSquared = static_cast<uint64_t>(minDistance * minDistance);
    const auto closestNode = getClosestNodeHelper(pos, minDistanceSquared, indexToIgnore);
    if (closestNode) {
        return closestNode->first;
    }

    return std::nullopt;
}

std::optional<std::pair<NodeIndex_t, uint64_t>> QuadTree::getClosestNodeHelper(
    QPoint pos, uint64_t minDistanceSquared, NodeIndex_t indexToIgnore) const {
    if (!m_boundary.intersects(QRect(pos.x() - NodeData::k_radius, pos.y() - NodeData::k_radius,
                                     2 * NodeData::k_radius, 2 * NodeData::k_radius))) {
        return std::nullopt;
    }

    NodeIndex_t closestNode = INVALID_NODE;
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].m_index == indexToIgnore) {
            continue;
        }

        const auto nodePos = m_nodes[i].m_position;
        const auto dx = static_cast<int64_t>(nodePos.x()) - pos.x();
        const auto dy = static_cast<int64_t>(nodePos.y()) - pos.y();
        const auto distanceSq = static_cast<uint64_t>(dx) * dx + static_cast<uint64_t>(dy) * dy;

        if (distanceSq < minDistanceSquared) {
            closestNode = m_nodes[i].m_index;
            minDistanceSquared = distanceSq;
        }
    }

    if (isSubdivided()) {
        auto value = m_northWest->getClosestNodeHelper(pos, minDistanceSquared, indexToIgnore);
        if (value && value->second < minDistanceSquared) {
            minDistanceSquared = value->second;
            closestNode = value->first;
        }

        value = m_northEast->getClosestNodeHelper(pos, minDistanceSquared, indexToIgnore);
        if (value && value->second < minDistanceSquared) {
            minDistanceSquared = value->second;
            closestNode = value->first;
        }

        value = m_southWest->getClosestNodeHelper(pos, minDistanceSquared, indexToIgnore);
        if (value && value->second < minDistanceSquared) {
            minDistanceSquared = value->second;
            closestNode = value->first;
        }

        value = m_southEast->getClosestNodeHelper(pos, minDistanceSquared, indexToIgnore);
        if (value && value->second < minDistanceSquared) {
            minDistanceSquared = value->second;
            closestNode = value->first;
        }
    }

    if (closestNode == INVALID_NODE) {
        return std::nullopt;
    }

    return std::make_pair(closestNode, minDistanceSquared);
}
