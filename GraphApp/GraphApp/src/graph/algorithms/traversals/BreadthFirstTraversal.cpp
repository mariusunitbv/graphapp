#include <pch.h>

#include "BreadthFirstTraversal.h"

BreadthFirstTraversal::BreadthFirstTraversal(Graph* graph) : ITimedAlgorithm(graph) {
    auto& graphManager = graph->getGraphManager();
    const auto nodeCount = graphManager.getNodesCount();

    m_nodesInfo.resize(nodeCount);

    graphManager.setAlgorithmPathColor(ANALYZED_EDGE, qRgb(60, 179, 113));
    graphManager.setAlgorithmPathColor(ANALYZING_EDGE, qRgb(255, 165, 0));
    graphManager.setAlgorithmPathColor(VISITED_EDGE, qRgb(70, 130, 180));
}

void BreadthFirstTraversal::start(NodeIndex_t startNode) {
    setStartNode(startNode);

    ITimedAlgorithm::start();
}

bool BreadthFirstTraversal::step() {
    if (m_traversalContainer.empty()) {
        return false;
    }

    auto& graphManager = m_graph->getGraphManager();
    const auto currentNode = m_traversalContainer.front();

    if (getNodeState(currentNode) != NodeData::State::ANALYZING) {
        setNodeState(currentNode, NodeData::State::ANALYZING);
        m_pseudocodeForm.highlight({9});

        return true;
    }

    bool addedNewNode = false;
    graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
        currentNode, [&](NodeIndex_t neighbour, CostType_t) {
            const auto neighbourState = getNodeState(neighbour);
            if (neighbourState == NodeData::State::UNVISITED) {
                setNodeState(neighbour, NodeData::State::VISITED);

                graphManager.addAlgorithmEdge(currentNode, neighbour, ANALYZING_EDGE);
                graphManager.addAlgorithmEdge(currentNode, neighbour, VISITED_EDGE);

                m_nodesInfo[neighbour].m_parentNode = currentNode;
                m_nodesInfo[neighbour].m_length = m_nodesInfo[currentNode].m_length + 1;
                m_traversalContainer.push_back(neighbour);

                addedNewNode = true;
            }
        });

    if (addedNewNode) {
        m_pseudocodeForm.highlight({10, 11, 12, 13});
        return true;
    }

    setNodeState(currentNode, NodeData::State::ANALYZED);
    m_pseudocodeForm.highlight({14});

    graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
        currentNode, [&](NodeIndex_t neighbour, CostType_t) {
            if (getNodeState(neighbour) != NodeData::State::UNVISITED) {
                graphManager.addAlgorithmEdge(currentNode, neighbour, ANALYZED_EDGE);
            }
        });

    m_traversalContainer.pop_front();

    return true;
}

void BreadthFirstTraversal::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM PBF;
(2)   BEGIN
(3)       U := N − {s};  V := {s};  W := ∅;
(4)       FOR toți y ∈ N DO p(y) := 0;
(5)       l(s) := 0;
(6)       FOR toți y ∈ U DO l(y) := ∞;
(7)       WHILE V ≠ ∅ DO
(8)       BEGIN
(9)           se selectează cel mai vechi nod x introdus în V;
(10)          FOR (x, y) ∈ A DO
(11)              IF y ∈ U
(12)              THEN U := U − {y};  V := V ∪ {y};  p(y) := x;
(13)                   l(y) := l(x) + 1;
(14)          V := V − {x};  W := W ∪ {x};
(15)      END;
(16) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void BreadthFirstTraversal::setStartNode(NodeIndex_t startNode) {
    m_traversalContainer.push_back(startNode);
    m_nodesInfo[startNode].m_length = 0;
}

void BreadthFirstTraversal::onFinishedAlgorithm() {
    auto& graphManager = m_graph->getGraphManager();
    graphManager.clearAlgorithmPath(ANALYZING_EDGE);
    graphManager.clearAlgorithmPath(VISITED_EDGE);

    ITimedAlgorithm::onFinishedAlgorithm();
}

void BreadthFirstTraversal::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList U, V, W, p, l;
    for (auto node : m_traversalContainer) {
        V << QString::number(node);
    }

    for (NodeIndex_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        const auto state = getNodeState(nodeIndex);
        switch (state) {
            case NodeData::State::UNVISITED:
                U << QString::number(nodeIndex);
                break;
            case NodeData::State::ANALYZED:
                W << QString::number(nodeIndex);
                break;
        }

        if (m_nodesInfo[nodeIndex].m_parentNode == INVALID_NODE) {
            p << "-";
        } else {
            p << QString::number(m_nodesInfo[nodeIndex].m_parentNode);
        }

        if (m_nodesInfo[nodeIndex].m_length == std::numeric_limits<uint32_t>::max()) {
            l << "∞";
        } else {
            l << QString::number(m_nodesInfo[nodeIndex].m_length);
        }
    }

    infoLines << "U = {" + U.join(", ") + "}";
    infoLines << "V = {" + V.join(", ") + "}";
    infoLines << "W = {" + W.join(", ") + "}";
    infoLines << "";
    infoLines << "p: [" + p.join(", ") + "]";
    infoLines << "l: [" + l.join(", ") + "]";

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}
