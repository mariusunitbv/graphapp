#include <pch.h>

#include "PathReconstruction.h"

PathReconstruction::PathReconstruction(Graph* graph)
    : ITimedAlgorithm(graph), m_genericTraversal(new GenericTraversal(graph)) {
    m_genericTraversal->setParent(this);
}

void PathReconstruction::start(NodeIndex_t start) {
    auto& graphManager = m_graph->getGraphManager();

    graphManager.disableAddingAlgorithmEdges();
    m_genericTraversal->setStartNode(start);
    m_genericTraversal->stepAll();
    graphManager.enableAddingAlgorithmEdges();

    setDefaultColorToVisitedNodes();

    bool ok = false;
    const auto destinationNode = QInputDialog::getInt(
        nullptr, "Destination Node", "Enter the destination node index:", 0, 0,
        static_cast<int>(m_graph->getGraphManager().getNodesCount() - 1), 1, &ok);
    if (!ok) {
        return cancelAlgorithm();
    }

    if (getNodeState(destinationNode) == NodeData::State::UNVISITED) {
        setNodeState(destinationNode, NodeData::State::UNREACHABLE);
        m_pseudocodeForm.highlight({10});

        QMessageBox::information(
            nullptr, "Path",
            QString("Node %1 is unreachable from Node %2.").arg(destinationNode).arg(start));

        emit finished();

        return;
    }

    m_currentNode = destinationNode;

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

    if (getNodeState(m_currentNode) != NodeData::State::ANALYZED) {
        setNodeState(m_currentNode, NodeData::State::ANALYZED);
        m_graph->getGraphManager().addAlgorithmEdge(m_currentNode, m_latestMarkedNode,
                                                    GenericTraversal::ANALYZED_EDGE);
        m_pseudocodeForm.highlight({6, 7});
        return true;
    }

    m_latestMarkedNode = m_currentNode;
    m_currentNode = m_genericTraversal->m_nodesInfo[m_currentNode].m_parentNode;
    m_pseudocodeForm.highlight({8});

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

void PathReconstruction::updateAlgorithmInfoText() const {}

void PathReconstruction::setDefaultColorToVisitedNodes() {
    auto& graphManager = m_graph->getGraphManager();
    for (NodeIndex_t i = 0; i < graphManager.getNodesCount(); ++i) {
        if (getNodeState(i) != NodeData::State::UNVISITED) {
            setNodeState(i, NodeData::State::NONE);
            graphManager.getNode(i).setFillColor(m_graph->getDefaultNodeColor());
        }
    }
}
