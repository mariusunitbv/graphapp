#pragma once

#include "../graph/Node.h"

class DisjointSet {
   public:
    explicit DisjointSet(size_t nodeCount);

    NodeIndex_t find(NodeIndex_t x);
    void unite(NodeIndex_t x, NodeIndex_t y);

   private:
    std::vector<NodeIndex_t> parent;
    std::vector<short> rank;
};
