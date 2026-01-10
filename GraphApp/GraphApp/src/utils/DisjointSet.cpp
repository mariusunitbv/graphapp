#include <pch.h>

#include "DisjointSet.h"

DisjointSet::DisjointSet(size_t nodeCount) : parent(nodeCount), rank(nodeCount, 0) {
    std::iota(parent.begin(), parent.end(), 0);
}

NodeIndex_t DisjointSet::find(NodeIndex_t x) {
    if (parent[x] != x) {
        parent[x] = find(parent[x]);
    }

    return parent[x];
}

void DisjointSet::unite(NodeIndex_t x, NodeIndex_t y) {
    const auto rootX = find(x);
    const auto rootY = find(y);

    if (rootX == rootY) {
        return;
    }

    if (rank[rootX] < rank[rootY]) {
        parent[rootX] = rootY;
    } else if (rank[rootX] > rank[rootY]) {
        parent[rootY] = rootX;
    } else {
        parent[rootY] = rootX;
        ++rank[rootX];
    }
}
