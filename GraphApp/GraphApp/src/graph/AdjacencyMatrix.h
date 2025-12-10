#pragma once

#include "Node.h"

class AdjacencyMatrix {
   public:
    AdjacencyMatrix() = default;

    AdjacencyMatrix(const AdjacencyMatrix&) = delete;
    AdjacencyMatrix& operator=(const AdjacencyMatrix&) = delete;

    AdjacencyMatrix(AdjacencyMatrix&& rhs) noexcept;
    AdjacencyMatrix& operator=(AdjacencyMatrix&& rhs) noexcept;

    void resize(size_t nodeCount);
    bool empty() const;

    void reset();
    void complete();

    void setEdge(NodeIndex_t i, NodeIndex_t j, int8_t cost);
    void clearEdge(NodeIndex_t i, NodeIndex_t j);

    bool hasEdge(NodeIndex_t i, NodeIndex_t j) const;
    int8_t getCost(NodeIndex_t i, NodeIndex_t j) const;

   private:
    static uint8_t encode(uint8_t cost);
    uint8_t read(NodeIndex_t i, NodeIndex_t j) const;

    NodeIndex_t m_nodeCount{0};
    std::vector<uint8_t> m_matrix{};
};
