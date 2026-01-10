#include <pch.h>

#include "GenericMST.h"

#include "../random/Random.h"

GenericMST::GenericMST(Graph* graph) : ITimedAlgorithm(graph) {
    const auto nodeCount = graph->getGraphManager().getNodesCount();

    m_components.resize(nodeCount);
    for (NodeIndex_t index = 0; index < nodeCount; ++index) {
        m_components[index].m_nodes.push_back(index);
    }

    m_disjointSet = std::make_unique<DisjointSet>(nodeCount);

    graph->getGraphManager().setAlgorithmPathColor(MST_EDGE, qRgb(60, 179, 113));
}

bool GenericMST::step() {
    const auto nodeCount = m_graph->getGraphManager().getNodesCount();
    if (m_currentIteration == nodeCount - 1) {
        return false;
    }

    if (m_currentComponentIndex == INVALID_COMPONENT) {
        pickRandomNonEmptyComponent();
        m_pseudocodeForm.highlight({9});
        return true;
    }

    auto& randomComponent = m_components[m_currentComponentIndex];
    NodeIndex_t representative = m_disjointSet->find(randomComponent.m_nodes.front());

    NodeIndex_t bestNode = INVALID_NODE, bestNeighbour = INVALID_NODE;
    CostType_t bestCost = std::numeric_limits<CostType_t>::max();

    for (const auto nodeIndex : randomComponent.m_nodes) {
        m_graph->getGraphManager().getGraphStorage()->forEachOutgoingEdgeWithOpposites(
            nodeIndex, [&](NodeIndex_t neighbourIndex, CostType_t cost) {
                const auto neighbourRepresentative = m_disjointSet->find(neighbourIndex);
                if (representative != neighbourRepresentative && cost < bestCost) {
                    bestCost = cost;
                    bestNode = nodeIndex;
                    bestNeighbour = neighbourIndex;
                }
            });
    }

    if (bestNode == INVALID_NODE || bestNeighbour == INVALID_NODE) {
        deselectCurrentComponent();
        QMessageBox::information(nullptr, "Generic MST",
                                 "The graph is disconnected. MST cannot be completed.",
                                 QMessageBox::Ok);

        return false;
    }

    deselectCurrentComponent();

    NodeIndex_t bestRepresentative = m_disjointSet->find(bestNeighbour);
    m_disjointSet->unite(representative, bestRepresentative);
    NodeIndex_t mergedRepresentative = m_disjointSet->find(representative);

    const auto srcComponentIndex =
        (mergedRepresentative == representative) ? bestRepresentative : representative;
    const auto tragetComponentIndex =
        (mergedRepresentative == representative) ? representative : bestRepresentative;
    auto& srcComponent = m_components[srcComponentIndex];
    auto& targetComponent = m_components[tragetComponentIndex];

    targetComponent.m_nodes.insert(targetComponent.m_nodes.end(), srcComponent.m_nodes.begin(),
                                   srcComponent.m_nodes.end());
    srcComponent.m_nodes.clear();

    targetComponent.m_edges.insert(targetComponent.m_edges.end(), srcComponent.m_edges.begin(),
                                   srcComponent.m_edges.end());
    targetComponent.m_edges.emplace_back(bestNode, bestNeighbour);
    srcComponent.m_edges.clear();

    m_graph->getGraphManager().addAlgorithmEdge(bestNode, bestNeighbour, MST_EDGE);
    setNodeState(bestNeighbour, NodeData::State::VISITED);
    m_pseudocodeForm.highlight({10, 11, 12, 13});
    ++m_currentIteration;

    return true;
}

void GenericMST::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM GENERIC;
(2) BEGIN
(3)     FOR i := 0 TO n DO
(4)     BEGIN
(5)         Nᵢ := {i}; A'ᵢ := ∅;
(6)     END;
(7)     FOR k := 0 TO n - 1 DO
(8)     BEGIN
(9)          se selectează Nᵢ cu Nᵢ ≠ ∅;
(10)         se selectează [y, ȳ] cu b[y, ȳ] := min{b[x, x̄] | x ∈ Nᵢ, x̄ ∉ Nᵢ};
(11)         se determină indicele j pentru care ȳ ∈ Nⱼ;
(12)         Nᵢ := Nᵢ ∪ Nⱼ; Nⱼ := ∅;
(13)         A'ᵢ := A'ᵢ ∪ A'ⱼ ∪ {[y, ȳ]}; A'ⱼ := ∅;     
(14)         IF k = n - 1
(15) 			THEN A' := A'ᵢ;
(16)     END;
(17) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void GenericMST::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList N_A;
    for (size_t componentIndex = 0; componentIndex < m_components.size(); ++componentIndex) {
        const auto& component = m_components[componentIndex];
        if (component.m_nodes.empty()) {
            continue;
        }

        QStringList nodesStr;
        for (const auto nodeIndex : component.m_nodes) {
            nodesStr << QString::number(nodeIndex);
        }

        QStringList edgesStr;
        for (const auto& edge : component.m_edges) {
            edgesStr << "[" + QString::number(edge.first) + ", " + QString::number(edge.second) +
                            "]";
        }

        N_A << "N_" + QString::number(componentIndex) + " = {" + nodesStr.join(", ") + "} A'_" +
                   QString::number(componentIndex) + " = {" + edgesStr.join(", ") + "}";
    }

    graphManager.setAlgorithmInfoText(N_A.join("\n"));
}

void GenericMST::pickRandomNonEmptyComponent() {
    do {
        m_currentComponentIndex =
            Random::get().getSize(0, m_graph->getGraphManager().getNodesCount() - 1);
    } while (m_components[m_currentComponentIndex].m_nodes.empty());

    for (const auto nodeIndex : m_components[m_currentComponentIndex].m_nodes) {
        setNodeState(nodeIndex, NodeData::State::ANALYZING);
    }
}

void GenericMST::deselectCurrentComponent() {
    for (const auto nodeIndex : m_components[m_currentComponentIndex].m_nodes) {
        setNodeState(nodeIndex, NodeData::State::VISITED);
    }

    m_currentComponentIndex = INVALID_COMPONENT;
}
