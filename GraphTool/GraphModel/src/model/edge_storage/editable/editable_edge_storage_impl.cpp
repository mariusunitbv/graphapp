module;
#include <pch.h>

module editable_edge_storage;

void EditableEdgeStorage::resize(uint32_t nodeCount) { m_edges.resize(nodeCount); }

void EditableEdgeStorage::onNodeAdded(NodeIndex_t nodeIndex) { m_edges.emplace_back(); }

void EditableEdgeStorage::addEdge(NodeIndex_t src, NodeIndex_t dest, int weight) {
    if (src >= m_edges.size() || dest >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    auto& entry = m_edges[src];
    if (entry.empty()) {
        entry.emplace_back(dest, weight);
        return;
    };

    auto it =
        std::lower_bound(entry.begin(), entry.end(), dest,
                         [](const Edge_t& edge, NodeIndex_t dest) { return edge.first < dest; });

    if (it != entry.end() && it->first == dest) {
        it->second = weight;
    } else {
        entry.insert(it, dest, weight);
    }
}

void EditableEdgeStorage::addEdgeFast(NodeIndex_t src, NodeIndex_t dest, int weight) {
    if (src >= m_edges.size() || dest >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    auto& entry = m_edges[src];
    if (!entry.empty()) {
        const auto lastNodeIndex = entry.back().first;
        if (lastNodeIndex == dest) {
            entry.back().second = weight;
            return;
        } else if (lastNodeIndex > dest) {
            m_edgesSorted = false;
        }
    }

    entry.emplace_back(dest, weight);
}

bool EditableEdgeStorage::hasEdge(NodeIndex_t src, NodeIndex_t dest, int* outWeight) const {
    if (src >= m_edges.size() || dest >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    const auto& entry = m_edges[src];
    auto it =
        std::lower_bound(entry.begin(), entry.end(), dest,
                         [](const Edge_t& edge, NodeIndex_t dest) { return edge.first < dest; });

    if (it != entry.end() && it->first == dest) {
        if (outWeight) {
            *outWeight = it->second;
        }
        return true;
    }

    return false;
}

void EditableEdgeStorage::removeEdge(NodeIndex_t src, NodeIndex_t dest) {
    if (src >= m_edges.size() || dest >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    auto& entry = m_edges[src];
    auto it =
        std::lower_bound(entry.begin(), entry.end(), dest,
                         [](const Edge_t& edge, NodeIndex_t dest) { return edge.first < dest; });

    if (it != entry.end() && it->first == dest) {
        entry.erase(it);
    }
}

void EditableEdgeStorage::remove(const common::MediumVector<NodeIndex_t>& indexRemap) {
    NodeIndex_t writeIndex = 0;
    for (NodeIndex_t readIndex = 0; readIndex < m_edges.size(); ++readIndex) {
        if (indexRemap[readIndex] == INVALID_NODE) {
            continue;
        }

        auto& readIndexList = m_edges[readIndex];
        common::algorithms::erase_if(readIndexList, [&indexRemap](Edge_t& edge) {
            const auto newDest = indexRemap[edge.first];
            if (newDest == INVALID_NODE) {
                return true;
            }

            edge.first = newDest;
            return false;
        });

        if (writeIndex != readIndex) {
            m_edges[writeIndex] = std::move(readIndexList);
        }

        ++writeIndex;
    }

    m_edges.resize(writeIndex);
}

void EditableEdgeStorage::reserveDegree(NodeIndex_t nodeIndex, uint32_t degree) {
    if (nodeIndex >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    m_edges[nodeIndex].reserve(degree);
}

void EditableEdgeStorage::sortEdges() {
    if (m_edgesSorted) {
        return;
    }

    for (auto& entry : m_edges) {
        std::sort(entry.begin(), entry.end(),
                  [](const Edge_t& a, const Edge_t& b) { return a.first < b.first; });

        common::algorithms::unique(
            entry, [](const Edge_t& a, const Edge_t& b) { return a.first == b.first; });
    }

    m_edgesSorted = true;
}

uint32_t EditableEdgeStorage::getNeighbourCount(NodeIndex_t src) const {
    if (src >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    return m_edges[src].size();
}

std::span<const EdgeStorage::Edge_t> EditableEdgeStorage::getNeighbours(NodeIndex_t src) const {
    if (src >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    return m_edges[src].span();
}

void EditableEdgeStorage::visitNeighbours(NodeIndex_t src, void* userData,
                                          bool (*callback)(void* userData, NodeIndex_t dest,
                                                           int weight),
                                          float percentage, bool distinct) const {
    if (src >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    const auto& entry = m_edges[src];
    const auto limit = static_cast<uint32_t>(entry.size() * percentage);

    for (auto i = 0u; i < limit; ++i) {
        const auto& [dest, weight] = entry[i];
        if (distinct && src > dest && hasEdge(dest, src)) {
            continue;
        }

        if (!callback(userData, dest, weight)) {
            return;
        }
    }
}

void EditableEdgeStorage::visitDistinctNeighbours(NodeIndex_t src, void* userData,
                                                  bool (*callback)(void* userData, NodeIndex_t dest,
                                                                   int weight, bool bothWays),
                                                  float percentage) const {
    if (src >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    const auto& entry = m_edges[src];
    const auto limit = static_cast<uint32_t>(entry.size() * percentage);

    for (auto i = 0u; i < limit; ++i) {
        const auto& [dest, weight] = entry[i];

        const auto bothWays = hasEdge(dest, src);
        if (src > dest && bothWays) {
            continue;
        }

        if (!callback(userData, dest, weight, bothWays)) {
            return;
        }
    }
}
