#include <pch.h>

#include "Dijkstra.h"

Dijkstra::Dijkstra(Graph* graph) : ITimedAlgorithm(graph) {
    auto& graphManager = m_graph->getGraphManager();

    m_nodesInfo.resize(graphManager.getNodesCount());

    graphManager.setAlgorithmPathColor(PATH_TO_TARGET, qRgb(60, 179, 113));
    graphManager.setAlgorithmPathColor(SHORTEST_PATHS, qRgb(70, 130, 180));
    graphManager.setAlgorithmPathColor(VISITED_PATH, qRgb(255, 165, 0));
}

void Dijkstra::start(NodeIndex_t startNode, NodeIndex_t targetNode) {
    m_startNode = startNode;
    m_targetNode = targetNode;

    m_nodesInfo[startNode].m_minCost = 0;
    m_minHeap.emplace(0, m_startNode);

    ITimedAlgorithm::start();
}

bool Dijkstra::step() {
    if (m_minHeap.empty()) {
        if (m_targetNode != INVALID_NODE) {
            markTargetUnreachable();
        } else {
            markPartialArborescence();
        }

        return false;
    }

    NodeIndex_t currentNode;
    int64_t currentCost;

    do {
        std::tie(currentCost, currentNode) = m_minHeap.top();
        m_minHeap.pop();
    } while (!m_minHeap.empty() && currentCost > m_nodesInfo[currentNode].m_minCost);

    if (currentCost > m_nodesInfo[currentNode].m_minCost) {
        if (m_targetNode != INVALID_NODE) {
            markTargetUnreachable();
        } else {
            markPartialArborescence();
        }

        return false;
    }

    auto& graphManager = m_graph->getGraphManager();
    if (currentNode == m_targetNode) {
        setNodeState(currentNode, NodeData::State::ANALYZED);
        graphManager.clearAlgorithmPath(VISITED_PATH);

        NodeIndex_t parent = m_nodesInfo[currentNode].m_parent;
        while (parent != INVALID_NODE) {
            graphManager.addAlgorithmEdge(parent, currentNode, PATH_TO_TARGET);

            currentNode = parent;
            parent = m_nodesInfo[currentNode].m_parent;

            setNodeState(currentNode, NodeData::State::ANALYZED);
        }

        const auto totalCost = m_nodesInfo[m_targetNode].m_minCost;
        QMessageBox::information(nullptr, "Dijkstra",
                                 QString("Path has finished.\nTotal cost: %1").arg(totalCost));

        return false;
    }

    if (getNodeState(currentNode) != NodeData::State::ANALYZING) {
        setNodeState(currentNode, NodeData::State::ANALYZING);
        m_pseudocodeForm.highlight({10, 11});
        m_minHeap.emplace(currentCost, currentNode);

        return true;
    }

    graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
        currentNode, [&](NodeIndex_t neighbour, CostType_t edgeCost) {
            const int64_t newCost = currentCost + edgeCost;

            if (newCost < m_nodesInfo[neighbour].m_minCost) {
                m_nodesInfo[neighbour].m_minCost = newCost;
                m_nodesInfo[neighbour].m_parent = currentNode;
                m_minHeap.emplace(newCost, neighbour);

                graphManager.addAlgorithmEdge(currentNode, neighbour, VISITED_PATH);
            }
        });

    setNodeState(currentNode, NodeData::State::VISITED);
    m_pseudocodeForm.highlight({12, 13, 14, 15, 16, 17});
    return true;
}

void Dijkstra::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM DIJKSTRA;
(2) BEGIN
(3)     W := N; d(s) := 0; p(s) := 0;
(4)     FOR y ∈ N − {s} DO
(5)     BEGIN
(6)         d(y) := ∞; p(y) := 0;
(7)     END;
(8)     WHILE W ≠ ∅ DO
(9)     BEGIN
(10)         se selectează un nod x ∈ W astfel încât d(x) este minimă;
(11)         W := W − {x};
(12)         FOR y ∈ W ∩ V+(x) DO
(13)             IF d(x) + b(x, y) < d(y) THEN
(14)             BEGIN
(15)                 d(y) := d(x) + b(x, y); 
(16)                 p(y) := x;
(17)             END;
(18)     END;
(19) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void Dijkstra::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList W, d, p;
    for (NodeIndex_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        const auto state = getNodeState(nodeIndex);
        if (state == NodeData::State::UNVISITED) {
            W << QString::number(nodeIndex);
        }

        if (m_nodesInfo[nodeIndex].m_minCost == std::numeric_limits<int64_t>::max()) {
            d << "∞";
        } else {
            d << QString::number(m_nodesInfo[nodeIndex].m_minCost);
        }

        if (m_nodesInfo[nodeIndex].m_parent == INVALID_NODE) {
            p << "-";
        } else {
            p << QString::number(m_nodesInfo[nodeIndex].m_parent);
        }
    }

    infoLines << "W = {" + W.join(", ") + "}";
    infoLines << "";
    infoLines << "d: [" + d.join(", ") + "]";
    infoLines << "p: [" + p.join(", ") + "]";

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void Dijkstra::resetForUndo() {
    for (auto& info : m_nodesInfo) {
        info.m_minCost = std::numeric_limits<int64_t>::max();
        info.m_parent = INVALID_NODE;
    }

    while (!m_minHeap.empty()) {
        m_minHeap.pop();
    }

    m_nodesInfo[m_startNode].m_minCost = 0;
    m_minHeap.emplace(0, m_startNode);
}

void Dijkstra::markTargetUnreachable() {
    setNodeState(m_targetNode, NodeData::State::UNREACHABLE);
    QMessageBox::information(nullptr, "Dijkstra",
                             "The target node is unreachable from the start node.",
                             QMessageBox::Ok);
}

void Dijkstra::markPartialArborescence() {
    auto& graphManager = m_graph->getGraphManager();
    graphManager.clearAlgorithmPath(VISITED_PATH);

    for (NodeIndex_t i = 0; i < m_nodesInfo.size(); ++i) {
        const auto parent = m_nodesInfo[i].m_parent;
        if (parent != INVALID_NODE) {
            graphManager.addAlgorithmEdge(parent, i, SHORTEST_PATHS);
        }
    }
}
