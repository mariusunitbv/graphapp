#include <pch.h>

#include "AdjacencyMatrix.h"

AdjacencyMatrix::AdjacencyMatrix(AdjacencyMatrix&& rhs) noexcept
    : m_nodeCount(rhs.m_nodeCount), m_matrix(std::move(rhs.m_matrix)) {}

AdjacencyMatrix& AdjacencyMatrix::operator=(AdjacencyMatrix&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    m_nodeCount = rhs.m_nodeCount;
    m_matrix = std::move(rhs.m_matrix);

    return *this;
}

void AdjacencyMatrix::resize(size_t nodeCount) {
    m_nodeCount = static_cast<NodeIndex_t>(nodeCount);
    m_matrix.resize(nodeCount * nodeCount, 0);
}

bool AdjacencyMatrix::empty() const { return m_matrix.empty(); }

void AdjacencyMatrix::reset() { std::fill(m_matrix.begin(), m_matrix.end(), 0); }

void AdjacencyMatrix::complete() { std::fill(m_matrix.begin(), m_matrix.end(), 0x80); }

void AdjacencyMatrix::setEdge(NodeIndex_t i, NodeIndex_t j, int8_t cost) {
    m_matrix[i * m_nodeCount + j] = encode(cost);
}

void AdjacencyMatrix::clearEdge(NodeIndex_t i, NodeIndex_t j) { m_matrix[i * m_nodeCount + j] = 0; }

bool AdjacencyMatrix::hasEdge(NodeIndex_t i, NodeIndex_t j) const { return read(i, j) >> 7; }

int8_t AdjacencyMatrix::getCost(NodeIndex_t i, NodeIndex_t j) const { return read(i, j) & 0x7F; }

uint8_t AdjacencyMatrix::encode(uint8_t cost) { return (1u << 7) | (cost & 0x7F); }

uint8_t AdjacencyMatrix::read(NodeIndex_t i, NodeIndex_t j) const {
    return m_matrix[i * m_nodeCount + j];
}
