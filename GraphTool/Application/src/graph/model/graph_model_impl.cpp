module;
#include <pch.h>

module graph_model;

void GraphModel::addNode(Vector2D worldPos) {
    if (m_nodes.size() >= NODE_LIMIT) {
        GAPP_THROW("Node limit reached");
    }

    const auto nodeArea = getNodeBoundingBox(worldPos);
    if (!WORLD_BOUNDS.contains(nodeArea)) {
        GAPP_THROW("Node position is out of world bounds");
    }

    if (updateDynamicBoundsIfNeeded(nodeArea)) {
        if (!m_bulkInsertMode) {
            rebuildGridMap();
        }
    }

    m_nodes.emplace_back(static_cast<NodeIndex_t>(m_nodes.size()), worldPos);

    if (!m_bulkInsertMode) {
        m_gridMap.insert(&m_nodes.back());
    }
}

void GraphModel::removeNodes(const std::unordered_set<NodeIndex_t>& nodes) {
    // Pre-removal stage.
    removeNodesFromGridMap(nodes);

    // Removal stage.
    const auto indexRemap = removeNodesAndCalculateIndexRemap(nodes);

    // Post-removal stage.
    m_gridMap.fixIndexesAfterNodeRemoval(indexRemap);
}

void GraphModel::reserveNodes(size_t nodeCount) { m_nodes.reserve(nodeCount); }

void GraphModel::beginBulkInsert() { m_bulkInsertMode = true; }

void GraphModel::endBulkInsert() {
    rebuildGridMap();
    m_bulkInsertMode = false;
}

NodeIndex_t GraphModel::getLastNodeIndex() const { return m_nodes.back().m_index; }

Node* GraphModel::getNode(NodeIndex_t index) { return &m_nodes[index]; }

const Node* GraphModel::getNode(NodeIndex_t index) const { return &m_nodes[index]; }

Node* GraphModel::getNodeAtPosition(Vector2D worldPos, bool firstOccurence, float minimumDistance,
                                    NodeIndex_t nodeToIgnore) {
    NodeIndex_t closestNodeIndex = INVALID_NODE;
    if (firstOccurence) {
        closestNodeIndex = m_gridMap.querySingleFast(
            m_nodes, worldPos, getNodeBoundingBox(worldPos), minimumDistance, nodeToIgnore);
    } else {
        closestNodeIndex = m_gridMap.querySingle(m_nodes, worldPos, minimumDistance, nodeToIgnore);
    }

    if (closestNodeIndex == INVALID_NODE) {
        return nullptr;
    }

    return &m_nodes[closestNodeIndex];
}

const Node* GraphModel::getNodeAtPosition(Vector2D worldPos, bool firstOccurence,
                                          float minimumDistance, NodeIndex_t nodeToIgnore) const {
    return const_cast<GraphModel*>(this)->getNodeAtPosition(worldPos, firstOccurence,
                                                            minimumDistance, nodeToIgnore);
}

const BoundingBox2D& GraphModel::getGraphBounds() const { return m_gridMap.getBounds(); }

std::vector<VisibleNode> GraphModel::queryNodes(const BoundingBox2D& area) const {
    return m_gridMap.query(m_nodes, area);
}

BoundingBox2D GraphModel::getNodeBoundingBox(Vector2D worldPos) {
    return BoundingBox2D{worldPos.m_x - NODE_RADIUS, worldPos.m_y - NODE_RADIUS,
                         worldPos.m_x + NODE_RADIUS, worldPos.m_y + NODE_RADIUS};
}

bool GraphModel::updateDynamicBoundsIfNeeded(const BoundingBox2D& bounds) {
    constexpr auto MARGIN_PADDING = 5.f;

    bool updated = false;

    auto dynamicBounds = m_gridMap.getBounds();
    if (bounds.m_min.m_x < dynamicBounds.m_min.m_x) {
        dynamicBounds.m_min.m_x = bounds.m_min.m_x - MARGIN_PADDING;
        updated = true;
    }

    if (bounds.m_min.m_y < dynamicBounds.m_min.m_y) {
        dynamicBounds.m_min.m_y = bounds.m_min.m_y - MARGIN_PADDING;
        updated = true;
    }

    if (bounds.m_max.m_x > dynamicBounds.m_max.m_x) {
        dynamicBounds.m_max.m_x = bounds.m_max.m_x + MARGIN_PADDING;
        updated = true;
    }

    if (bounds.m_max.m_y > dynamicBounds.m_max.m_y) {
        dynamicBounds.m_max.m_y = bounds.m_max.m_y + MARGIN_PADDING;
        updated = true;
    }

    if (updated) {
        dynamicBounds.clamp(WORLD_BOUNDS);
        m_gridMap.setBounds(dynamicBounds);
    }

    return updated;
}

void GraphModel::removeNodesFromGridMap(const std::unordered_set<NodeIndex_t>& nodes) {
    for (const auto index : nodes) {
        m_gridMap.remove(index, getNodeBoundingBox(m_nodes[index].m_worldPos));
    }
}

std::vector<NodeIndex_t> GraphModel::removeNodesAndCalculateIndexRemap(
    const std::unordered_set<NodeIndex_t>& nodes) {
    std::vector<NodeIndex_t> indexRemap(m_nodes.size());

    NodeIndex_t writeIndex = 0;
    for (NodeIndex_t readIndex = 0; readIndex < m_nodes.size(); ++readIndex) {
        if (nodes.contains(m_nodes[readIndex].m_index)) {
            indexRemap[readIndex] = INVALID_NODE;
            continue;
        }

        if (writeIndex != readIndex) {
            m_nodes[writeIndex] = std::move(m_nodes[readIndex]);
            m_nodes[writeIndex].setIndex(writeIndex);
        }

        indexRemap[readIndex] = writeIndex++;
    }

    m_nodes.resize(writeIndex);
    return indexRemap;
}

void GraphModel::rebuildGridMap() {
    m_gridMap.allocateCells();
    for (const auto& node : m_nodes) {
        m_gridMap.insert(&node);
    }
}
