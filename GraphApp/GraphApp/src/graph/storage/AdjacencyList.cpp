#include <pch.h>

#include "AdjacencyList.h"

IGraphStorage::Type AdjacencyList::type() const { return Type::ADJACENCY_LIST; }

void AdjacencyList::resize(size_t nodeCount) { m_adjacencyList.resize(nodeCount); }

void AdjacencyList::addEdge(NodeIndex_t start, NodeIndex_t end, CostType_t cost) {
    auto& neighbours = m_adjacencyList[start];

    if (!neighbours.empty() && neighbours.back().first < end) {
        neighbours.emplace_back(end, cost);
        return;
    }

    auto it = std::lower_bound(
        neighbours.begin(), neighbours.end(), end,
        [](const auto& neighbour, NodeIndex_t value) { return neighbour.first < value; });

    if (it != neighbours.end() && it->first == end) {
        it->second = cost;
        return;
    }

    neighbours.insert(it, std::make_pair(end, cost));
}

void AdjacencyList::removeEdge(NodeIndex_t start, NodeIndex_t end) {
    auto& neighbours = m_adjacencyList[start];
    auto it = getNeighbour(start, end);
    if (it == neighbours.end()) {
        throw std::runtime_error{"Tried removing an edge that doesn't exist!"};
    }

    neighbours.erase(it);
}

std::optional<CostType_t> AdjacencyList::getEdge(NodeIndex_t start, NodeIndex_t end) const {
    if (start >= m_adjacencyList.size()) {
        return std::nullopt;
    }

    const auto it = getNeighbour(start, end);
    if (it == m_adjacencyList[start].end()) {
        return std::nullopt;
    }

    return it->second;
}

void AdjacencyList::forEachOutgoingEdge(
    NodeIndex_t node, const std::function<void(NodeIndex_t, CostType_t)>& callback) const {
    if (node >= m_adjacencyList.size()) {
        return;
    }

    for (const auto& [neighbour, cost] : m_adjacencyList[node]) {
        if (node >= neighbour && getEdge(neighbour, node)) {
            continue;
        }

        callback(neighbour, cost);
    }
}

void AdjacencyList::forEachOutgoingEdgeWithOpposites(
    NodeIndex_t node, const std::function<void(NodeIndex_t, CostType_t)>& callback) const {
    if (node >= m_adjacencyList.size()) {
        return;
    }

    for (const auto& [neighbour, cost] : m_adjacencyList[node]) {
        callback(neighbour, cost);
    }
}

void AdjacencyList::recomputeBeforeRemovingNodes(
    size_t oldNodeCount, const std::set<NodeIndex_t, std::greater<NodeIndex_t>>& selectedNodes) {
    std::vector<NodeIndex_t> indexRemap(oldNodeCount, INVALID_NODE);
    for (NodeIndex_t oldIndex = 0, newIndex = 0; oldIndex < oldNodeCount; ++oldIndex) {
        if (!selectedNodes.contains(oldIndex)) {
            indexRemap[oldIndex] = newIndex++;
        }
    }

    AdjacencyList_t newAdjacencyList;
    newAdjacencyList.resize(oldNodeCount - selectedNodes.size());

    for (size_t start = 0; start < oldNodeCount; ++start) {
        NodeIndex_t newStart = indexRemap[start];
        if (newStart == INVALID_NODE) {
            continue;
        }

        const auto& neighbours = m_adjacencyList[start];
        Neighbours_t newNeighbours;
        newNeighbours.reserve(neighbours.size());

        for (const auto& [end, cost] : neighbours) {
            NodeIndex_t newEnd = indexRemap[end];
            if (newEnd != INVALID_NODE) {
                newNeighbours.emplace_back(newEnd, cost);
            }
        }

        newAdjacencyList[newStart] = std::move(newNeighbours);
    }

    m_adjacencyList = std::move(newAdjacencyList);
}

void AdjacencyList::recomputeAfterAddingNode(size_t newNodeCount) { resize(newNodeCount); }

AdjacencyList::Neighbours_t::iterator AdjacencyList::getNeighbour(NodeIndex_t start,
                                                                  NodeIndex_t end) {
    auto& neighbours = m_adjacencyList[start];
    auto it = std::lower_bound(
        neighbours.begin(), neighbours.end(), end,
        [](const auto& neighbour, NodeIndex_t dest) { return neighbour.first < dest; });
    if (it == neighbours.end() || it->first != end) {
        return neighbours.end();
    }

    return it;
}

AdjacencyList::Neighbours_t::const_iterator AdjacencyList::getNeighbour(NodeIndex_t start,
                                                                        NodeIndex_t end) const {
    const auto& neighbours = m_adjacencyList[start];
    auto it = std::lower_bound(
        neighbours.cbegin(), neighbours.cend(), end,
        [](const auto& neighbour, NodeIndex_t dest) { return neighbour.first < dest; });
    if (it == neighbours.cend() || it->first != end) {
        return neighbours.cend();
    }

    return it;
}
