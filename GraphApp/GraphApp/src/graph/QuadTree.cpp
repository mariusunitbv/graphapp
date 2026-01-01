#include <pch.h>

#include "QuadTree.h"

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

void QuadTree::getNodesInArea(const QRect& area, std::vector<bool>& visitMask,
                              std::vector<NodeIndex_t>& nodes) const {
    if (!m_boundary.intersects(area)) {
        return;
    }

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const auto index = m_nodes[i].m_index;
        const auto& nodePos = m_nodes[i].m_position;
        const QRect nodeArea(nodePos.x() - NodeData::k_radius, nodePos.y() - NodeData::k_radius,
                             2 * NodeData::k_radius, 2 * NodeData::k_radius);

        if (!visitMask[index] && area.intersects(nodeArea)) {
            nodes.push_back(index);
            visitMask[index] = true;
        }
    }

    if (!isSubdivided()) {
        return;
    }

    m_northWest->getNodesInArea(area, visitMask, nodes);
    m_northEast->getNodesInArea(area, visitMask, nodes);
    m_southWest->getNodesInArea(area, visitMask, nodes);
    m_southEast->getNodesInArea(area, visitMask, nodes);
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

    m_northWest.reset();
    m_northEast.reset();
    m_southWest.reset();
    m_southEast.reset();
}

void QuadTree::subdivide() {
    const auto x = m_boundary.x();
    const auto y = m_boundary.y();
    const auto w = m_boundary.width() / 2;
    const auto h = m_boundary.height() / 2;

    if (!canSubdivide()) {
        throw std::runtime_error("Cannot subdivide QuadTree further.");
    }

    m_northWest = std::make_unique<QuadTree>();
    m_northWest->setBoundary(QRect(x, y, w, h));

    m_northEast = std::make_unique<QuadTree>();
    m_northEast->setBoundary(QRect(x + w, y, w, h));

    m_southWest = std::make_unique<QuadTree>();
    m_southWest->setBoundary(QRect(x, y + h, w, h));

    m_southEast = std::make_unique<QuadTree>();
    m_southEast->setBoundary(QRect(x + w, y + h, w, h));
}

bool QuadTree::isSubdivided() const { return m_northWest != nullptr; }

bool QuadTree::canSubdivide() const {
    return m_boundary.width() / 2 > NodeData::k_radius &&
           m_boundary.height() / 2 > NodeData::k_radius;
}

const QuadTreePtr_t& QuadTree::getNorthWest() const { return m_northWest; }

const QuadTreePtr_t& QuadTree::getNorthEast() const { return m_northEast; }

const QuadTreePtr_t& QuadTree::getSouthWest() const { return m_southWest; }

const QuadTreePtr_t& QuadTree::getSouthEast() const { return m_southEast; }

bool QuadTree::intersectsAnotherNode(QPoint pos, float minDistance,
                                     NodeIndex_t indexToIgnore) const {
    if (!m_boundary.intersects(QRect(pos.x() - NodeData::k_radius, pos.y() - NodeData::k_radius,
                                     2 * NodeData::k_radius, 2 * NodeData::k_radius))) {
        return false;
    }

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].m_index == indexToIgnore) {
            continue;
        }

        const auto nodePos = m_nodes[i].m_position;

        const int64_t dx = nodePos.x() - pos.x();
        const int64_t dy = nodePos.y() - pos.y();

        if (dx * dx + dy * dy < (minDistance * minDistance)) {
            return true;
        }
    }

    if (!isSubdivided()) {
        return false;
    }

    if (m_northWest->intersectsAnotherNode(pos, minDistance, indexToIgnore)) {
        return true;
    }

    if (m_northEast->intersectsAnotherNode(pos, minDistance, indexToIgnore)) {
        return true;
    }

    if (m_southWest->intersectsAnotherNode(pos, minDistance, indexToIgnore)) {
        return true;
    }

    if (m_southEast->intersectsAnotherNode(pos, minDistance, indexToIgnore)) {
        return true;
    }

    return false;
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
