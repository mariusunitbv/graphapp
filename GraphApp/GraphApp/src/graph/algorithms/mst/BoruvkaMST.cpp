#include <pch.h>

#include "BoruvkaMST.h"

BoruvkaMST::BoruvkaMST(Graph* graph) : ITimedAlgorithm(graph) {
    const auto nodeCount = graph->getGraphManager().getNodesCount();

    m_components.resize(nodeCount);
    for (NodeIndex_t index = 0; index < nodeCount; ++index) {
        m_components[index].m_nodes.push_back(index);
    }

    m_mstEdges.reserve(nodeCount - 1);
    m_disjointSet = std::make_unique<DisjointSet>(nodeCount);

    graph->getGraphManager().setAlgorithmPathColor(MST_EDGE, qRgb(60, 179, 113));
}

bool BoruvkaMST::step() {
    if (m_components.size() == 1) {
        return false;
    }

    auto& graphManager = m_graph->getGraphManager();
    if (m_shouldPickEdges) {
        std::vector<std::pair<NodeIndex_t, NodeIndex_t>> chosenEdges;
        for (auto& component : m_components) {
            NodeIndex_t bestNode = INVALID_NODE, bestNeighbour = INVALID_NODE;
            NodeIndex_t bestRepresentative = INVALID_NODE;
            std::optional<CostType_t> bestCost;

            for (auto nodeIndex : component.m_nodes) {
                NodeIndex_t representative = m_disjointSet->find(nodeIndex);

                graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
                    nodeIndex, [&](NodeIndex_t neighbourIndex, CostType_t cost) {
                        NodeIndex_t neighbourRepresentative = m_disjointSet->find(neighbourIndex);
                        const bool inSameComponent = representative == neighbourRepresentative;
                        if (inSameComponent) {
                            return;
                        }

                        if (!bestCost || (bestCost && cost < bestCost.value())) {
                            bestNode = nodeIndex;
                            bestNeighbour = neighbourIndex;
                            bestRepresentative = neighbourRepresentative;
                            bestCost = cost;
                        }
                    });
            }

            if (!bestCost) {
                QMessageBox::information(nullptr, "Boruvka MST",
                                         "The graph is disconnected. MST cannot be completed.",
                                         QMessageBox::Ok);

                return false;
            }

            chosenEdges.emplace_back(bestNode, bestNeighbour);
        }

        m_shouldPickEdges = false;
        for (auto& [u, v] : chosenEdges) {
            if (m_disjointSet->find(u) != m_disjointSet->find(v)) {
                m_disjointSet->unite(u, v);
                m_mstEdges.emplace_back(u, v);

                graphManager.addAlgorithmEdge(u, v, MST_EDGE);

                setNodeState(u, NodeData::State::VISITED);
                setNodeState(v, NodeData::State::VISITED);
            }
        }

        m_pseudocodeForm.highlight({8, 9, 10, 11, 12, 13});
        return true;
    }

    m_components.clear();
    std::unordered_map<NodeIndex_t, std::vector<NodeIndex_t>> newComponents;
    for (NodeIndex_t i = 0; i < graphManager.getNodesCount(); ++i) {
        NodeIndex_t rep = m_disjointSet->find(i);
        newComponents[rep].push_back(i);
    }

    for (auto& [_, nodes] : newComponents) {
        m_components.emplace_back(std::move(nodes));
    }

    m_shouldPickEdges = true;

    m_pseudocodeForm.highlight({14, 15});
    return true;
}

void BoruvkaMST::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM BORUVKA;
(2) BEGIN
(3)   FOR i := 1 TO n DO
(4)      Nᵢ := {i};
(5)   A' := ∅; M := {N₁,...,Nₙ};
(6)   WHILE |A'| < n−1 DO
(7)   BEGIN
(8)     FOR Nᵢ ∈ M DO
(9)     BEGIN
(10)       se selectează [y,y'] cu b[y,y'] := min{b[x,x'] | x ∈ Nᵢ, x' ∉ Nᵢ};
(11)       se determină indicele j pentru care y ∈ Nⱼ;
(12)       A' := A' ∪ {[y,y']};
(13)    END;
(14)    FOR Nᵢ ∈ M DO BEGIN Nᵢ := Nᵢ ∪ Nⱼ; Nⱼ := ∅; END;
(15)    M := { ..., Nᵢ, ..., Nⱼ, ... | Nᵢ ∩ Nⱼ = ∅, [Nᵢ = N] };
(16)  END;
(17) END;
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void BoruvkaMST::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList M, N_i;
    for (size_t i = 0; i < m_components.size(); ++i) {
        const auto& component = m_components[i];

        QStringList nodeIndicesStr;
        for (const auto nodeIndex : component.m_nodes) {
            nodeIndicesStr << QString::number(nodeIndex);
        }

        N_i << QString("N%1 = {%2}").arg(i).arg(nodeIndicesStr.join(", "));
        M << QString("N%1").arg(i);
    }

    QStringList A;
    for (const auto [node, neighbour] : m_mstEdges) {
        A << QString("[%1, %2]").arg(node).arg(neighbour);
    }

    infoLines << "M: {" + M.join(", ") + "}";
    infoLines << "A': {" + A.join(", ") + "}";
    infoLines << N_i.join("\n");

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void BoruvkaMST::resetForUndo() {
    const auto nodeCount = m_graph->getGraphManager().getNodesCount();

    m_components.resize(nodeCount);
    for (uint32_t i = 0; i < nodeCount; ++i) {
        m_components[i].m_nodes = std::vector<NodeIndex_t>{i};
    }

    m_mstEdges.clear();
    m_disjointSet = std::make_unique<DisjointSet>(nodeCount);
    m_shouldPickEdges = true;
}
