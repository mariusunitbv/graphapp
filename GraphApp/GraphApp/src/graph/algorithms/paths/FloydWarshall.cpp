#include <pch.h>

#include "FloydWarshall.h"

FloydWarshall::FloydWarshall(Graph* graph) : ITimedAlgorithm(graph) {
    auto& graphManager = graph->getGraphManager();
    const auto nodeCount = graphManager.getNodesCount();

    m_distanceMatrix.resize(nodeCount * nodeCount, MAX_COST);
    m_parentMatrix.resize(nodeCount * nodeCount, INVALID_NODE);

    for (NodeIndex_t i = 0; i < nodeCount; ++i) {
        graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
            i, [&](NodeIndex_t j, CostType_t cost) {
                m_distanceMatrix[i * nodeCount + j] = cost;
                m_parentMatrix[i * nodeCount + j] = i;
            });

        m_distanceMatrix[i * nodeCount + i] = 0;
        m_parentMatrix[i * nodeCount + i] = INVALID_NODE;
    }

    graphManager.setAlgorithmPathColor(SHORTEST_PATH, qRgb(60, 179, 113));
    graphManager.setAlgorithmPathColor(CURRENT_PATH, qRgb(204, 166, 63));
    graphManager.setAlgorithmPathColor(VISITED_PATH, qRgb(51, 82, 222));
}

bool FloydWarshall::step() {
    if (m_stepDelay == 0) {
        runParallelized();
        return false;
    }

    auto& graphManager = m_graph->getGraphManager();
    const auto nodeCount = m_graph->getGraphManager().getNodesCount();

    if (!m_firstStep) {
        m_prevI = m_currentI;
        m_prevJ = m_currentJ;
        m_prevK = m_currentK;

        if (++m_currentJ == nodeCount) {
            m_currentJ = 0;
            if (++m_currentI == nodeCount) {
                m_currentI = 0;
                if (++m_currentK == nodeCount) {
                    return false;
                }
            }
        }
    }

    uncolorPreviousNodes();
    colorNodesForCurrentStep();
    m_pseudocodeForm.highlight({11, 12, 13});

    const auto& distIK = m_distanceMatrix[m_currentI * nodeCount + m_currentK];
    const auto& distKJ = m_distanceMatrix[m_currentK * nodeCount + m_currentJ];
    auto& distIJ = m_distanceMatrix[m_currentI * nodeCount + m_currentJ];

    if (distIK != MAX_COST && distKJ != MAX_COST) {
        const auto newDist = distIK + distKJ;
        if (distIJ == MAX_COST || newDist < distIJ) {
            distIJ = newDist;

            m_parentMatrix[m_currentI * nodeCount + m_currentJ] =
                m_parentMatrix[m_currentK * nodeCount + m_currentJ];

            if (m_currentI != m_currentK) {
                graphManager.addAlgorithmEdge(m_currentI, m_currentK, SHORTEST_PATH);
            }

            if (m_currentK != m_currentJ) {
                graphManager.addAlgorithmEdge(m_currentK, m_currentJ, SHORTEST_PATH);
            }

            m_pseudocodeForm.highlight({14, 15});

            if (m_currentI == m_currentJ && newDist < 0) {
                m_negativeLoopCycle = true;
                QMessageBox::warning(nullptr, "Negative Cycle Detected",
                                     "A negative cycle has been detected in the graph. "
                                     "The Floyd-Warshall algorithm cannot proceed further.");
                return false;
            }
        }
    }

    m_firstStep = false;
    return true;
}

void FloydWarshall::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM FLOYD–WARSHALL;
(2) BEGIN
(3)   FOR i := 1 TO n DO
(4)     FOR j := 1 TO n DO
(5)     BEGIN
(6)       dᵢⱼ := bᵢⱼ;
(7)       IF i ≠ j AND dᵢⱼ < ∞
(8)         THEN pᵢⱼ := i
(9)         ELSE pᵢⱼ := 0
(10)    END;
(11)  FOR k := 1 TO n DO
(12)    FOR i := 1 TO n DO
(13)      FOR j := 1 TO n DO
(14)        IF dᵢₖ + dₖⱼ < dᵢⱼ
(15)          THEN BEGIN dᵢⱼ := dᵢₖ + dₖⱼ; pᵢⱼ := pₖⱼ; END;
(16) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void FloydWarshall::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList distanceMatrixLines, parentMatrixLines;
    for (size_t i = 0; i < nodeCount; ++i) {
        QStringList distanceRow, parentRow;
        for (size_t j = 0; j < nodeCount; ++j) {
            size_t index = i * nodeCount + j;

            if (m_distanceMatrix[index] != MAX_COST) {
                distanceRow << QString::number(m_distanceMatrix[index]);
            } else {
                distanceRow << "∞";
            }

            if (m_parentMatrix[index] != INVALID_NODE) {
                parentRow << QString::number(m_parentMatrix[index]);
            } else {
                parentRow << "-";
            }
        }

        distanceMatrixLines << "|" + distanceRow.join("\t") + "|";
        parentMatrixLines << "|" + parentRow.join("\t") + "|";
    }

    infoLines << "k = " + QString::number(m_currentK) + " (Intermediate node)";
    infoLines << "i = " + QString::number(m_currentI) + " (Source node)";
    infoLines << "j = " + QString::number(m_currentJ) + " (Target node)";
    infoLines << "Distance Matrix:";
    infoLines << distanceMatrixLines;
    infoLines << "Parent Matrix:";
    infoLines << parentMatrixLines;

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void FloydWarshall::colorNodesForCurrentStep() {
    auto& graphManager = m_graph->getGraphManager();
    const auto nodeCount = graphManager.getNodesCount();

    graphManager.clearAlgorithmPath(SHORTEST_PATH);
    graphManager.clearAlgorithmPath(CURRENT_PATH);

    setNodeState(m_currentK, NodeData::State::ANALYZING);
    setNodeState(m_currentJ, NodeData::State::VISITED);
    setNodeState(m_currentI, NodeData::State::ANALYZED);

    if (m_currentI != m_currentK && graphManager.hasNeighbour(m_currentI, m_currentK)) {
        graphManager.addAlgorithmEdge(m_currentI, m_currentK, CURRENT_PATH);
        graphManager.addAlgorithmEdge(m_currentI, m_currentK, VISITED_PATH);
    }

    if (m_currentK != m_currentJ && graphManager.hasNeighbour(m_currentK, m_currentJ)) {
        graphManager.addAlgorithmEdge(m_currentK, m_currentJ, CURRENT_PATH);
        graphManager.addAlgorithmEdge(m_currentK, m_currentJ, VISITED_PATH);
    }
}

void FloydWarshall::uncolorPreviousNodes() {
    if (m_prevI == INVALID_NODE) {
        return;
    }

    setNodeState(m_prevI, NodeData::State::UNVISITED);
    setNodeState(m_prevJ, NodeData::State::UNVISITED);
    setNodeState(m_prevK, NodeData::State::UNVISITED);
}

void FloydWarshall::runParallelized() {
    const auto nodeCount = m_graph->getGraphManager().getNodesCount();
    const auto range = std::views::iota(NodeIndex_t{0}, static_cast<NodeIndex_t>(nodeCount));
    for (NodeIndex_t k = 0; k < nodeCount; ++k) {
        std::for_each(std::execution::par, range.begin(), range.end(), [&](NodeIndex_t i) {
            if (m_negativeLoopCycle) {
                return;
            }

            const auto& distIK = m_distanceMatrix[i * nodeCount + k];
            if (distIK == MAX_COST) {
                return;
            }

            for (NodeIndex_t j = 0; j < nodeCount; ++j) {
                const auto& distKJ = m_distanceMatrix[k * nodeCount + j];
                if (distKJ == MAX_COST) {
                    continue;
                }

                auto& distIJ = m_distanceMatrix[i * nodeCount + j];
                const auto newDist = distIK + distKJ;
                if (distIJ == MAX_COST || newDist < distIJ) {
                    distIJ = newDist;
                    m_parentMatrix[i * nodeCount + j] = m_parentMatrix[k * nodeCount + j];

                    if (i == j && newDist < 0) {
                        m_negativeLoopCycle = true;
                        return;
                    }
                }
            }
        });
    }

    if (m_negativeLoopCycle) {
        QMessageBox::warning(nullptr, "Negative Cycle Detected",
                             "A negative cycle has been detected in the graph. "
                             "The Floyd-Warshall algorithm cannot proceed further.");
    }
}
