module;
#include <pch.h>

module editable_edge_storage;

void EditableEdgeStorage::resize(size_t nodeCount) { m_edges.resize(nodeCount); }

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
        entry.insert(it, {dest, weight});
    }
}

void EditableEdgeStorage::addEdgeFast(NodeIndex_t src, NodeIndex_t dest, int weight) {
    if (src >= m_edges.size() || dest >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    m_edges[src].emplace_back(dest, weight);
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
    GAPP_THROW("Not implemented!");
}

void EditableEdgeStorage::remove(const std::vector<NodeIndex_t>& indexRemap) {
    NodeIndex_t writeIndex = 0;
    for (NodeIndex_t readIndex = 0; readIndex < m_edges.size(); ++readIndex) {
        if (indexRemap[readIndex] == INVALID_NODE) {
            continue;
        }

        auto& readIndexList = m_edges[readIndex];
        std::erase_if(readIndexList, [&indexRemap](Edge_t& edge) {
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

void EditableEdgeStorage::sortEdges() {
    for (auto& entry : m_edges) {
        std::sort(entry.begin(), entry.end(),
                  [](const Edge_t& a, const Edge_t& b) { return a.first < b.first; });
    }
}

size_t EditableEdgeStorage::getNeighbourCount(NodeIndex_t src) const {
    if (src >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    return m_edges[src].size();
}

void EditableEdgeStorage::visitNeighbours(NodeIndex_t src, void* userData,
                                          void (*callback)(void* userData, NodeIndex_t dest,
                                                           int weight),
                                          float percentage, bool distinct) const {
    if (src >= m_edges.size()) {
        GAPP_THROW("Forgotten to call onNodeAdded(), size mismatch.");
    }

    const auto& entry = m_edges[src];
    const auto limit = static_cast<size_t>(entry.size() * percentage);

    for (size_t i = 0; i < limit; ++i) {
        const auto& [dest, weight] = entry[i];
        if (distinct && src >= dest && hasEdge(dest, src)) {
            continue;
        }

        callback(userData, dest, weight);
    }
}
