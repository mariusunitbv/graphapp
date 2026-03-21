module;
#include <pch.h>

module graph_model;

import editable_edge_storage;

GraphModel::GraphModel() : m_edgeStorage(std::make_unique<EditableEdgeStorage>()) {}

int GraphModel::getGridMapCellSize() const { return GridMap::CELL_SIZE; }

void GraphModel::addNode(Vector2D worldPos) {
    if (m_nodes.size() >= NODE_LIMIT) {
        GAPP_THROW("Node limit reached");
    }

    worldPos = Vector2D::trunc(worldPos);
    if (!WORLD_BOUNDS.contains(worldPos)) {
        GAPP_THROW("Node position is out of world bounds");
    }

    if (updateDynamicBoundsIfNeeded({worldPos, worldPos})) {
        if (!m_bulkInsertMode) {
            rebuildGridMap();
        }
    } else {
        if (m_bulkInsertMode) {
            m_gridMap.incrementNodeCountInCells(worldPos);
        }
    }

    m_nodes.emplace_back(worldPos);

    if (!m_bulkInsertMode) {
        m_gridMap.insert(&m_nodes.back(), getLastNodeIndex());
        m_edgeStorage->onNodeAdded(getLastNodeIndex());
    }
}

void GraphModel::removeSelectedNodes() {
    const auto indexRemap = removeSelectedNodesAndCalculateIndexRemap();

    m_gridMap.remove(indexRemap);
    m_edgeStorage->remove(indexRemap);
}

void GraphModel::reserveNodes(uint32_t nodeCount) {
    if (!m_bulkInsertMode) {
        GAPP_THROW("reserveNodes can only be called in bulk insert mode");
    }

    m_nodes.reserve(nodeCount);
    m_edgeStorage->resize(nodeCount);
}

void GraphModel::reserveArea(const BoundingBox2D& area) {
    updateDynamicBoundsIfNeeded(area);
    m_gridMap.allocateCells();
}

void GraphModel::beginBulkInsert() { m_bulkInsertMode = true; }

void GraphModel::endBulkInsert() {
    m_gridMap.reserveNodeCountInCells();
    for (NodeIndex_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex) {
        m_gridMap.insert(&m_nodes[nodeIndex], nodeIndex);
    }

    m_edgeStorage->sortEdges();

    m_bulkInsertMode = false;
}

uint32_t GraphModel::getNodeCount() const { return static_cast<uint32_t>(m_nodes.size()); }

NodeIndex_t GraphModel::getLastNodeIndex() const {
    if (m_nodes.empty()) {
        return INVALID_NODE;
    }

    return static_cast<NodeIndex_t>(m_nodes.size() - 1);
}

NodeIndex_t GraphModel::getNodeIndex(const Node* node) const {
    return static_cast<NodeIndex_t>(std::distance(m_nodes.data(), node));
}

Node* GraphModel::getNode(NodeIndex_t index) { return &m_nodes[index]; }

const Node* GraphModel::getNode(NodeIndex_t index) const { return &m_nodes[index]; }

Node* GraphModel::getNodeAtPosition(Vector2D worldPos, float minimumDistance, bool firstOccurence,
                                    NodeIndex_t nodeToIgnore) {
    NodeIndex_t closestNodeIndex = INVALID_NODE;
    if (firstOccurence) {
        closestNodeIndex =
            m_gridMap.querySingleFast(m_nodes.span(), worldPos, minimumDistance, nodeToIgnore);
    } else {
        closestNodeIndex =
            m_gridMap.querySingle(m_nodes.span(), worldPos, minimumDistance, nodeToIgnore);
    }

    if (closestNodeIndex == INVALID_NODE) {
        return nullptr;
    }

    return &m_nodes[closestNodeIndex];
}

const Node* GraphModel::getNodeAtPosition(Vector2D worldPos, float minimumDistance,
                                          bool firstOccurence, NodeIndex_t nodeToIgnore) const {
    return const_cast<GraphModel*>(this)->getNodeAtPosition(worldPos, minimumDistance,
                                                            firstOccurence, nodeToIgnore);
}

const BoundingBox2D& GraphModel::getGraphBounds() const { return m_gridMap.getBounds(); }

uint32_t GraphModel::estimateNodeCountInArea(const BoundingBox2D& area) const {
    return m_gridMap.estimateNodeCountInArea(area);
}

void GraphModel::addEdge(NodeIndex_t src, NodeIndex_t dest, int weight) {
    if (m_bulkInsertMode) {
        m_edgeStorage->addEdgeFast(src, dest, weight);
    } else {
        m_edgeStorage->addEdge(src, dest, weight);
    }
}

void GraphModel::addEdgeFast(NodeIndex_t src, NodeIndex_t dest, int weight) {
    m_edgeStorage->addEdgeFast(src, dest, weight);
}

void GraphModel::removeEdge(NodeIndex_t src, NodeIndex_t dest) {
    m_edgeStorage->removeEdge(src, dest);
}

void GraphModel::reserveDegree(NodeIndex_t nodeIndex, uint32_t degree) {
    m_edgeStorage->reserveDegree(nodeIndex, degree);
}

void GraphModel::sortEdges() { m_edgeStorage->sortEdges(); }

uint32_t GraphModel::getNodeDegree(NodeIndex_t index) const {
    return m_edgeStorage->getNeighbourCount(index);
}

void GraphModel::visitNeighbours(NodeIndex_t src, void* userData,
                                 bool (*callback)(void* userData, NodeIndex_t dest, int weight),
                                 float percentage, bool distinct) const {
    m_edgeStorage->visitNeighbours(src, userData, callback, percentage, distinct);
}

void GraphModel::resizeEdgeStorage(uint32_t nodeCount) { m_edgeStorage->resize(nodeCount); }

bool GraphModel::updateDynamicBoundsIfNeeded(const BoundingBox2D& bounds) {
    bool updated = false;

    auto dynamicBounds = m_gridMap.getBounds();
    if (bounds.m_min.m_x < dynamicBounds.m_min.m_x) {
        dynamicBounds.m_min.m_x = bounds.m_min.m_x;
        updated = true;
    }

    if (bounds.m_min.m_y < dynamicBounds.m_min.m_y) {
        dynamicBounds.m_min.m_y = bounds.m_min.m_y;
        updated = true;
    }

    if (bounds.m_max.m_x > dynamicBounds.m_max.m_x) {
        dynamicBounds.m_max.m_x = bounds.m_max.m_x;
        updated = true;
    }

    if (bounds.m_max.m_y > dynamicBounds.m_max.m_y) {
        dynamicBounds.m_max.m_y = bounds.m_max.m_y;
        updated = true;
    }

    if (updated) {
        if (m_bulkInsertMode && m_gridMap.isAllocated()) {
            GAPP_THROW(
                "Dynamic bounds cannot be updated in bulk insert mode after the grid map has been "
                "allocated");
        }

        dynamicBounds.clamp(WORLD_BOUNDS);
        m_gridMap.setBounds(dynamicBounds);
    }

    return updated;
}

void GraphModel::rebuildGridMap() {
    m_gridMap.allocateCells();
    for (NodeIndex_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex) {
        m_gridMap.insert(&m_nodes[nodeIndex], nodeIndex);
    }
}

common::MediumVector<NodeIndex_t> GraphModel::removeSelectedNodesAndCalculateIndexRemap() {
    common::MediumVector<NodeIndex_t> indexRemap(m_nodes.size());

    NodeIndex_t writeIndex = 0;
    for (NodeIndex_t readIndex = 0; readIndex < m_nodes.size(); ++readIndex) {
        if (m_nodes[readIndex].isSelected()) {
            indexRemap[readIndex] = INVALID_NODE;
            continue;
        }

        if (writeIndex != readIndex) {
            m_nodes[writeIndex] = m_nodes[readIndex];
        }

        indexRemap[readIndex] = writeIndex++;
    }

    m_nodes.resize(writeIndex);
    return indexRemap;
}
