module;
#include <pch.h>

module dijkstra;

AlgorithmType Dijkstra::getType() const { return AlgorithmType::DIJKSTRA; }

const char* Dijkstra::getName() const { return "Dijkstra"; }

void Dijkstra::initialize() {
    setNodesState(NodeState::NONE);
    notifyPseudocodeEvent("init");

    m_nodesInfo.resize(m_model->getNodeCount());

    m_nodesInfo[m_sourceNode].m_minCost = 0;
    m_minHeap.emplace(0, m_sourceNode);

    m_currentMinNode = m_lastRelaxedNeighbour = INVALID_NODE;
    m_neighbourIndex = 0;
}

void Dijkstra::restartAlgorithm() {
    setNodesState(NodeState::NONE);
    notifyPseudocodeEvent("init");

    for (auto& info : m_nodesInfo) {
        info.m_minCost = std::numeric_limits<int64_t>::max();
        info.m_parent = INVALID_NODE;
    }

    while (!m_minHeap.empty()) {
        m_minHeap.pop();
    }

    m_nodesInfo[m_sourceNode].m_minCost = 0;
    m_minHeap.emplace(0, m_sourceNode);

    m_currentMinNode = m_lastRelaxedNeighbour = INVALID_NODE;
    m_neighbourIndex = 0;
}

bool Dijkstra::stepAlgorithm() {
    if (m_currentMinNode == INVALID_NODE) {
        while (!m_minHeap.empty()) {
            const auto [cost, node] = m_minHeap.top();
            m_minHeap.pop();
            if (getNodeState(node) != NodeState::VISITED) {
                m_currentMinNode = node;
                break;
            }
        }
    }

    if (m_currentMinNode == INVALID_NODE) {
        if (m_targetNode != INVALID_NODE) {
            markTargetUnreachable();
        }

        return false;
    } else if (m_currentMinNode == m_targetNode) {
        markShortestPath();
        return false;
    }

    if (getNodeState(m_currentMinNode) != NodeState::ANALYZING) {
        setNodeState(m_currentMinNode, NodeState::ANALYZING);
        notifyPseudocodeEvent("analyzing");

        return true;
    }

    const auto& neighbours = m_model->getNodeEdges(m_currentMinNode);
    while (m_neighbourIndex < neighbours.size()) {
        const auto [neighbour, weight] = neighbours[m_neighbourIndex++];
        if (getNodeState(neighbour) == NodeState::VISITED || m_currentMinNode == neighbour) {
            continue;
        }

        const auto newCost = m_nodesInfo[m_currentMinNode].m_minCost + weight;
        if (newCost < m_nodesInfo[neighbour].m_minCost) {
            setNodeState(neighbour, NodeState::RELAXED);
            notifyPseudocodeEvent("relax");

            if (m_lastRelaxedNeighbour != INVALID_NODE &&
                getNodeState(m_lastRelaxedNeighbour) == NodeState::RELAXED) {
                setNodeState(m_lastRelaxedNeighbour, NodeState::NONE);
            }

            m_nodesInfo[neighbour].m_minCost = newCost;
            m_nodesInfo[neighbour].m_parent = m_currentMinNode;
            m_minHeap.emplace(newCost, neighbour);
            m_lastRelaxedNeighbour = neighbour;

            return true;
        }
    }

    setNodeState(m_currentMinNode, NodeState::VISITED);
    notifyPseudocodeEvent("analyzed");

    m_currentMinNode = m_lastRelaxedNeighbour = INVALID_NODE;
    m_neighbourIndex = 0;

    return true;
}

IAlgorithm::ExecutionInfo_t Dijkstra::getExecutionInfo() const {
    ExecutionInfo_t info;

    info.emplace_back("W (Unprocessed nodes)", "");
    info.emplace_back("p (Parent nodes)", "");
    info.emplace_back("d (Distance vector)", "");

    const auto finishedPath =
        m_targetNode != INVALID_NODE && getNodeState(m_targetNode) == NodeState::VISITED;
    if (finishedPath) {
        info.emplace_back("Shortest path", "Cost: ");

        uint32_t pathNodeCount = 0;
        auto& shortestPath = info.back().second;
        shortestPath += std::to_string(m_nodesInfo[m_targetNode].m_minCost) + "\n[";

        for (const auto& [src, dest] : m_highlightedEdges) {
            if (++pathNodeCount % 10 == 0) {
                shortestPath += '\n';
            }

            shortestPath += std::to_string(src) + ", ";
        }
        shortestPath += std::to_string(m_targetNode);
        shortestPath += "]";
    }

    auto& W = info[0].second;
    auto& p = info[1].second;
    auto& d = info[2].second;

    uint32_t wCount = 0, pCount = 0, dCount = 0;

    W = "W = {";
    p = "p = [";
    d = "d = [";

    for (NodeIndex_t i = 0; i < m_model->getNodeCount(); ++i) {
        if (getNodeState(i) != NodeState::VISITED) {
            if (wCount++ > 0) {
                W += ", ";
            }
            if (wCount % 10 == 0) {
                W += '\n';
            }

            W += std::to_string(i);
        }

        if (pCount++ > 0) {
            p += ", ";
        }
        if (pCount % 10 == 0) {
            p += '\n';
        }

        if (m_nodesInfo[i].m_parent == INVALID_NODE) {
            p += '-';
        } else {
            p += std::to_string(m_nodesInfo[i].m_parent);
        }

        if (dCount++ > 0) {
            d += ", ";
        }
        if (dCount % 10 == 0) {
            d += '\n';
        }

        if (m_nodesInfo[i].m_minCost == std::numeric_limits<int64_t>::max()) {
            d += "∞";
        } else {
            d += std::to_string(m_nodesInfo[i].m_minCost);
        }
    }

    return info;
}

void Dijkstra::markTargetUnreachable() {
    setNodeState(m_targetNode, NodeState::UNREACHABLE);
    common::Logger::get().warning("Dijkstra: Target node {} is unreachable from source node {}",
                                  m_targetNode, m_sourceNode);
}

void Dijkstra::markShortestPath() {
    if (!m_highlightedEdges.empty()) {
        return;
    }

    setNodeState(m_targetNode, NodeState::VISITED);

    NodeIndex_t current = m_targetNode;
    NodeIndex_t parent = m_nodesInfo[m_targetNode].m_parent;
    while (parent != INVALID_NODE) {
        m_highlightedEdges.emplace_back(parent, current);

        current = parent;
        parent = m_nodesInfo[current].m_parent;
    }

    std::reverse(m_highlightedEdges.begin(), m_highlightedEdges.end());
}
