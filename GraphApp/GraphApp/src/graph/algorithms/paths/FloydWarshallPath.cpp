#include <pch.h>

#include "FloydWarshallPath.h"

FloydWarshallPath::FloydWarshallPath(Graph* graph)
    : ITimedAlgorithm(graph), m_floydWarshallAlgorithm(new FloydWarshall(graph)) {
    m_floydWarshallAlgorithm->setParent(this);
}

void FloydWarshallPath::start() {
    auto& graphManager = m_graph->getGraphManager();

    graphManager.disableAddingAlgorithmEdges();
    m_floydWarshallAlgorithm->stepAll();
    graphManager.enableAddingAlgorithmEdges();

    const auto selectedNodes = graphManager.getTwoSelectedNodes();
    m_startNodeIndex = selectedNodes->first;
    m_endNodeIndex = selectedNodes->second;
    m_currentNodeIndex = m_endNodeIndex;

    ITimedAlgorithm::start();
}

bool FloydWarshallPath::step() {
    if (m_floydWarshallAlgorithm->m_negativeLoopCycle) {
        return false;
    }

    if (m_currentNodeIndex == m_startNodeIndex) {
        QMessageBox::information(
            nullptr, "Path",
            QString("Path reconstruction completed. Total path cost: %1.").arg(m_totalPathCost));
        return false;
    }

    const auto nodeCount = m_graph->getGraphManager().getNodesCount();
    const auto parent =
        m_floydWarshallAlgorithm->m_parentMatrix[m_startNodeIndex * nodeCount + m_currentNodeIndex];
    if (parent == INVALID_NODE) {
        setNodeState(m_startNodeIndex, NodeData::State::UNREACHABLE);
        QMessageBox::information(nullptr, "Path",
                                 QString("Node %1 is unreachable from Node %2.")
                                     .arg(m_startNodeIndex)
                                     .arg(m_endNodeIndex));
        return false;
    }

    setNodeState(m_currentNodeIndex, NodeData::State::ANALYZED);
    setNodeState(parent, NodeData::State::ANALYZED);

    m_graph->getGraphManager().addAlgorithmEdge(parent, m_currentNodeIndex,
                                                FloydWarshall::SHORTEST_PATH);
    m_totalPathCost +=
        m_floydWarshallAlgorithm
            ->m_distanceMatrix[m_startNodeIndex * nodeCount + m_currentNodeIndex] -
        m_floydWarshallAlgorithm->m_distanceMatrix[m_startNodeIndex * nodeCount + parent];
    m_currentNodeIndex = parent;

    m_pseudocodeForm.highlight({6, 7});
    return true;
}

void FloydWarshallPath::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM DRUM;
(2) BEGIN
(3)   k := n; xₖ := j;
(4)   WHILE xₖ ≠ i DO
(5)   BEGIN
(6)     xₖ₋₁ := pᵢₓₖ;
(7)     k := k − 1;
(8)   END;
(9) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void FloydWarshallPath::updateAlgorithmInfoText() const {}
