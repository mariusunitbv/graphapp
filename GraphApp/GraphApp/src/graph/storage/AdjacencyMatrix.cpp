#include <pch.h>

#include "AdjacencyMatrix.h"

IGraphStorage::Type AdjacencyMatrix::type() const { return Type::ADJACENCY_MATRIX; }

void AdjacencyMatrix::resize(size_t nodeCount) {
    m_nodeCount = nodeCount;
    m_matrix.resize(nodeCount * nodeCount, 0);
}

void AdjacencyMatrix::addEdge(NodeIndex_t i, NodeIndex_t j, CostType_t cost) {
    m_matrix[i * m_nodeCount + j] = encode(cost);
}

void AdjacencyMatrix::removeEdge(NodeIndex_t i, NodeIndex_t j) {
    m_matrix[i * m_nodeCount + j] = 0;
}

bool AdjacencyMatrix::hasEdge(NodeIndex_t i, NodeIndex_t j) const { return read(i, j) & FLAG_BIT; }

CostType_t AdjacencyMatrix::getCost(NodeIndex_t i, NodeIndex_t j) const {
    return read(i, j) & COST_MASK;
}

void AdjacencyMatrix::forEachOutgoingEdge(
    NodeIndex_t node, const std::function<void(NodeIndex_t, CostType_t)>& callback) const {
    for (NodeIndex_t i = 0; i < m_nodeCount; ++i) {
        if (node >= i && hasEdge(i, node)) {
            continue;
        }

        if (hasEdge(node, i)) {
            callback(i, getCost(node, i));
        }
    }
}

void AdjacencyMatrix::forEachOutgoingEdgeWithOpposites(
    NodeIndex_t node, const std::function<void(NodeIndex_t, CostType_t)>& callback) const {
    for (NodeIndex_t i = 0; i < m_nodeCount; ++i) {
        if (hasEdge(node, i)) {
            callback(i, getCost(node, i));
        }
    }
}

void AdjacencyMatrix::recomputeBeforeRemovingNodes(
    size_t oldNodeCount, const std::set<NodeIndex_t, std::greater<NodeIndex_t>>& selectedNodes) {
    size_t newNodeCount = m_nodeCount - selectedNodes.size();

    std::vector<UnsignedCostType_t> newMatrix;
    newMatrix.resize(newNodeCount * newNodeCount, 0);

    std::vector<NodeIndex_t> indexRemap(m_nodeCount, INVALID_NODE);
    for (NodeIndex_t oldIndex = 0, newIndex = 0; oldIndex < m_nodeCount; ++oldIndex) {
        if (!selectedNodes.contains(oldIndex)) {
            indexRemap[oldIndex] = newIndex++;
        }
    }

    const auto indices = std::views::iota(NodeIndex_t{0}, static_cast<NodeIndex_t>(m_nodeCount));
    std::for_each(std::execution::par, indices.begin(), indices.end(), [&](NodeIndex_t i) {
        const auto new_i = indexRemap[i];
        if (new_i == INVALID_NODE) {
            return;
        }

        for (NodeIndex_t j = 0; j < m_nodeCount; ++j) {
            const auto new_j = indexRemap[j];
            if (new_j == INVALID_NODE || !hasEdge(i, j)) {
                continue;
            }

            newMatrix[new_i * newNodeCount + new_j] = encode(getCost(i, j));
        }
    });

    m_nodeCount = newNodeCount;
    m_matrix = std::move(newMatrix);
}

void AdjacencyMatrix::recomputeAfterAddingNode(size_t newNodeCount) {
    std::vector<UnsignedCostType_t> newMatrix;
    newMatrix.resize(newNodeCount * newNodeCount, 0);

    const auto indices = std::views::iota(NodeIndex_t{0}, static_cast<NodeIndex_t>(m_nodeCount));
    std::for_each(std::execution::par, indices.begin(), indices.end(), [&](NodeIndex_t i) {
        for (NodeIndex_t j = 0; j < m_nodeCount; ++j) {
            if (hasEdge(i, j)) {
                newMatrix[i * newNodeCount + j] = encode(getCost(i, j));
            }
        }
    });

    m_nodeCount = newNodeCount;
    m_matrix = std::move(newMatrix);
}

size_t AdjacencyMatrix::getMemoryUsage() const {
    return sizeof(AdjacencyMatrix) + m_nodeCount * m_nodeCount * sizeof(CostType_t);
}

void AdjacencyMatrix::complete() { std::fill(m_matrix.begin(), m_matrix.end(), FLAG_BIT); }

AdjacencyMatrix::UnsignedCostType_t AdjacencyMatrix::encode(CostType_t cost) {
    return FLAG_BIT | (cost & COST_MASK);
}

AdjacencyMatrix::UnsignedCostType_t AdjacencyMatrix::read(NodeIndex_t i, NodeIndex_t j) const {
    return m_matrix[i * m_nodeCount + j];
}
