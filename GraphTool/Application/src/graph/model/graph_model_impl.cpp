module;
#include <pch.h>

module graph_model;

import quadtree;

Node::Node(NodeIndex_t index, Vector2D worldPos) : m_worldPos{worldPos} { setIndex(index); }

void Node::setIndex(NodeIndex_t index) {
    m_index = index;

    size_t charCount = 1;
    while (index >= 10) {
        index /= 10;
        ++charCount;
    }

    if (charCount > sizeof(m_labelBuffer) - 1) {
        GAPP_THROW("Node index exceeds label buffer size");
    }

    std::snprintf(m_labelBuffer, charCount + 1, "%u", m_index);
}

bool Node::hasColor() const { return !(m_red == 0 && m_green == 0 && m_blue == 0); }

uint32_t Node::getRGB() const {
    constexpr auto ALPHA_MASK = 0xFF000000;
    return ALPHA_MASK | (static_cast<uint32_t>(m_blue) << 16) |
           (static_cast<uint32_t>(m_green) << 8) | static_cast<uint32_t>(m_red);
}

void GraphModel::addNode(Vector2D worldPos) {
    if (m_nodes.size() >= NODE_LIMIT) {
        GAPP_THROW("Node limit reached");
    }

    const auto nodeArea = getNodeBoundingBox(worldPos);
    if (updateDynamicBoundsIfNeeded(nodeArea)) {
        if (!m_bulkInsertMode) {
            rebuildQuadTree();
        }
    }

    m_nodes.emplace_back(static_cast<NodeIndex_t>(m_nodes.size()), worldPos);

    if (!m_bulkInsertMode) {
        m_quadTree.insert(&m_nodes.back());
    }
}

void GraphModel::removeNodes(const std::unordered_set<NodeIndex_t>& nodes) {
    removeNodesAndCalculateIndexRemap(nodes);

    m_indexRemapAfterRemoval.clear();
    m_indexRemapAfterRemoval.shrink_to_fit();
}

void GraphModel::reserveNodes(size_t nodeCount) { m_nodes.reserve(nodeCount); }

void GraphModel::beginBulkInsert() { m_bulkInsertMode = true; }

void GraphModel::endBulkInsert() {
    rebuildQuadTree();
    m_bulkInsertMode = false;
}

Node* GraphModel::getNode(NodeIndex_t index) { return &m_nodes[index]; }

const Node* GraphModel::getNode(NodeIndex_t index) const { return &m_nodes[index]; }

const std::vector<Node>& GraphModel::getNodes() const { return m_nodes; }

NodeIndex_t GraphModel::getNodeIndexAfterRemoval(NodeIndex_t originalIndex) const {
    if (m_indexRemapAfterRemoval.empty()) {
        GAPP_THROW("No nodes have been removed, index remap is not available");
    }

    if (originalIndex >= m_indexRemapAfterRemoval.size()) {
        GAPP_THROW("Original index out of bounds for index remap");
    }

    return m_indexRemapAfterRemoval[originalIndex];
}

const QuadTree* GraphModel::getQuadTree() const { return &m_quadTree; }

BoundingBox2D GraphModel::getNodeBoundingBox(Vector2D worldPos) {
    return BoundingBox2D{worldPos.m_x - NODE_RADIUS, worldPos.m_y - NODE_RADIUS,
                         worldPos.m_x + NODE_RADIUS, worldPos.m_y + NODE_RADIUS};
}

bool GraphModel::updateDynamicBoundsIfNeeded(const BoundingBox2D& bounds) {
    constexpr auto MARGIN_PADDING = 5.f;

    auto& dynamicBounds = m_quadTree.getBoundsMutable();
    if (bounds.m_min.m_x < dynamicBounds.m_min.m_x) {
        dynamicBounds.m_min.m_x = bounds.m_min.m_x - MARGIN_PADDING;
        return true;
    }

    if (bounds.m_min.m_y < dynamicBounds.m_min.m_y) {
        dynamicBounds.m_min.m_y = bounds.m_min.m_y - MARGIN_PADDING;
        return true;
    }

    if (bounds.m_max.m_x > dynamicBounds.m_max.m_x) {
        dynamicBounds.m_max.m_x = bounds.m_max.m_x + MARGIN_PADDING;
        return true;
    }

    if (bounds.m_max.m_y > dynamicBounds.m_max.m_y) {
        dynamicBounds.m_max.m_y = bounds.m_max.m_y + MARGIN_PADDING;
        return true;
    }

    return false;
}

void GraphModel::removeNodesAndCalculateIndexRemap(const std::unordered_set<NodeIndex_t>& nodes) {
    m_indexRemapAfterRemoval.resize(m_nodes.size());

    NodeIndex_t writeIndex = 0;
    for (NodeIndex_t readIndex = 0; readIndex < m_nodes.size(); ++readIndex) {
        if (nodes.contains(m_nodes[readIndex].m_index)) {
            m_indexRemapAfterRemoval[readIndex] = INVALID_NODE;
            continue;
        }

        if (writeIndex != readIndex) {
            m_nodes[writeIndex] = std::move(m_nodes[readIndex]);
            m_nodes[writeIndex].setIndex(writeIndex);
        }

        m_indexRemapAfterRemoval[readIndex] = writeIndex++;
    }

    m_nodes.resize(writeIndex);
}

void GraphModel::rebuildQuadTree() {
    m_quadTree.clear();
    for (const auto& node : m_nodes) {
        m_quadTree.insert(&node);
    }
}
