#include <pch.h>

#include "PathReconstruction.h"

PathReconstruction::PathReconstruction(Graph* graph)
    : ITimedAlgorithm(graph), m_genericTraversal(new GenericTraversal(graph)) {
    m_genericTraversal->setParent(this);
}

void PathReconstruction::start(NodeIndex_t start, NodeIndex_t end) {
    auto& graphManager = m_graph->getGraphManager();

    graphManager.disableAddingAlgorithmEdges();
    m_genericTraversal->setStartNode(start);
    m_genericTraversal->stepAll();
    graphManager.enableAddingAlgorithmEdges();

    setDefaultColorToVisitedNodes();

    if (getNodeState(end) == NodeData::State::UNVISITED) {
        setNodeState(end, NodeData::State::UNREACHABLE);
        m_pseudocodeForm.highlight({10});

        QMessageBox::information(
            nullptr, "Path", QString("Node %1 is unreachable from Node %2.").arg(end).arg(start));

        emit finished();

        return;
    }

    m_endNode = m_currentNode = end;

    ITimedAlgorithm::start();
}

bool PathReconstruction::step() {
    if (m_currentNode == INVALID_NODE) {
        return false;
    }

    if (m_firstStep) {
        m_pseudocodeForm.highlight({3});
        setNodeState(m_currentNode, NodeData::State::ANALYZED);
        m_firstStep = false;
        return true;
    }

    const auto parent = m_genericTraversal->m_nodesInfo[m_currentNode].m_parentNode;
    if (parent == INVALID_NODE) {
        return false;
    }

    setNodeState(parent, NodeData::State::ANALYZED);
    m_graph->getGraphManager().addAlgorithmEdge(parent, m_currentNode,
                                                GenericTraversal::ANALYZED_EDGE);

    m_currentNode = parent;
    m_pseudocodeForm.highlight({6, 7, 8});

    return true;
}

void PathReconstruction::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROCEDURA DRUM(G, s, y);
(2)   BEGIN
(3)       se tipărește y;
(4)       WHILE p(y) ≠ -1 DO
(5)       BEGIN
(6)           x := p(y);
(7)           se tipărește x;
(8)           y := x
(9)       END;
(10) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void PathReconstruction::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList p;
    for (NodeIndex_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        if (m_genericTraversal->m_nodesInfo[nodeIndex].m_parentNode == INVALID_NODE) {
            p << "-";
        } else {
            p << QString::number(m_genericTraversal->m_nodesInfo[nodeIndex].m_parentNode);
        }
    }

    infoLines << "p: [" + p.join(", ") + "]";
    infoLines << "";
    infoLines << "y = " + QString::number(m_currentNode);

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void PathReconstruction::resetForUndo() {
    setDefaultColorForUndo();

    m_currentNode = m_endNode;
    m_firstStep = true;
}

void PathReconstruction::setDefaultColorToVisitedNodes() {
    auto& graphManager = m_graph->getGraphManager();
    for (NodeIndex_t i = 0; i < graphManager.getNodesCount(); ++i) {
        if (getNodeState(i) != NodeData::State::UNVISITED) {
            setNodeState(i, NodeData::State::NONE);
            graphManager.getNode(i).setFillColor(m_graph->getDefaultNodeColor());
        }
    }
}

void PathReconstruction::setDefaultColorForUndo() {
    auto& graphManager = m_graph->getGraphManager();

    for (NodeIndex_t i = 0; i < m_genericTraversal->m_nodesInfo.size(); ++i) {
        const auto& nodeInfo = m_genericTraversal->m_nodesInfo[i];
        if (nodeInfo.m_visitOrder != 0) {
            setNodeState(i, NodeData::State::NONE);
            graphManager.getNode(i).setFillColor(m_graph->getDefaultNodeColor());
        }
    }
}
