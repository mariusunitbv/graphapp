#include <pch.h>

#include "AdjacencyList.h"

AdjacencyList::AdjacencyList()
    : m_adjacencyList(0, std::hash<NodeIndex_t>{}, std::equal_to<NodeIndex_t>{},
                      TrackingAllocator<AdjacencyListValue_t>(&m_memoryTracker)) {}

IGraphStorage::Type AdjacencyList::type() const { return Type::ADJACENCY_LIST; }

void AdjacencyList::resize(size_t nodeCount) { m_adjacencyList.reserve(nodeCount); }

void AdjacencyList::addEdge(NodeIndex_t start, NodeIndex_t end, CostType_t cost) {
    auto [neighboursIt, inserted] = m_adjacencyList.try_emplace(
        start, Neighbours_t{TrackingAllocator<Neighbour_t>(&m_memoryTracker)});

    auto& neighbours = neighboursIt->second;
    auto it = std::lower_bound(
        neighbours.begin(), neighbours.end(), end,
        [](const auto& neighbour, NodeIndex_t value) { return neighbour.first < value; });

    neighbours.insert(it, std::make_pair(end, cost));
}

void AdjacencyList::removeEdge(NodeIndex_t start, NodeIndex_t end) {
    if (!m_adjacencyList.contains(start)) {
        return;
    }

    auto& neighbours = m_adjacencyList.at(start);
    auto it = getNeighbour(start, end);
    if (it == neighbours.end()) {
        throw std::runtime_error{"Tried removing an edge that doesn't exist!"};
    }

    neighbours.erase(getNeighbour(start, end));
}

bool AdjacencyList::hasEdge(NodeIndex_t start, NodeIndex_t end) const {
    if (!m_adjacencyList.contains(start)) {
        return false;
    }

    return getNeighbour(start, end) != m_adjacencyList.at(start).end();
}

CostType_t AdjacencyList::getCost(NodeIndex_t start, NodeIndex_t end) const {
    const auto& it = getNeighbour(start, end);
    if (it == m_adjacencyList.at(start).end()) {
        throw std::runtime_error("Tried getting cost for a non-existing edge!");
    }

    return it->second;
}

void AdjacencyList::forEachOutgoingEdge(
    NodeIndex_t node, const std::function<void(NodeIndex_t, CostType_t)>& callback) const {
    if (!m_adjacencyList.contains(node)) {
        return;
    }

    for (const auto& [neighbour, cost] : m_adjacencyList.at(node)) {
        if (node >= neighbour && hasEdge(neighbour, node)) {
            continue;
        }

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

    AdjacencyList_t newAdjacencyList(0, std::hash<NodeIndex_t>{}, std::equal_to<NodeIndex_t>{},
                                     TrackingAllocator<AdjacencyListValue_t>(&m_memoryTracker));
    newAdjacencyList.reserve(m_adjacencyList.size());

    for (const auto& [start, neighbours] : m_adjacencyList) {
        NodeIndex_t newStart = indexRemap[start];
        if (newStart == INVALID_NODE) {
            continue;
        }

        Neighbours_t newNeighbours{TrackingAllocator<Neighbour_t>(&m_memoryTracker)};
        newNeighbours.reserve(neighbours.size());

        for (const auto& [end, cost] : neighbours) {
            NodeIndex_t newEnd = indexRemap[end];
            if (newEnd != INVALID_NODE) {
                newNeighbours.emplace_back(newEnd, cost);
            }
        }

        if (!newNeighbours.empty()) {
            newAdjacencyList.emplace(newStart, std::move(newNeighbours));
        }
    }

    m_adjacencyList = std::move(newAdjacencyList);
}

void AdjacencyList::recomputeAfterAddingNode(size_t newNodeCount) {}

size_t AdjacencyList::getMemoryUsage() const { return m_memoryTracker.m_usedBytes; }

AdjacencyList::Neighbours_t::iterator AdjacencyList::getNeighbour(NodeIndex_t start,
                                                                  NodeIndex_t end) {
    auto& neighbours = m_adjacencyList.at(start);
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
    const auto& neighbours = m_adjacencyList.at(start);
    auto it = std::lower_bound(
        neighbours.cbegin(), neighbours.cend(), end,
        [](const auto& neighbour, NodeIndex_t dest) { return neighbour.first < dest; });
    if (it == neighbours.cend() || it->first != end) {
        return neighbours.cend();
    }

    return it;
}
