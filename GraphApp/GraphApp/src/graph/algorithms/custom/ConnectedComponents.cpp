#include <pch.h>

#include "ConnectedComponents.h"

#include "../random/Random.h"

static constexpr auto CC_PSEUDOCODE_PRIORITY = 1;

ConnectedComponents::ConnectedComponents(Graph* graph) : DepthFirstTotalTraversal(graph) {
    connect(this, &IAlgorithm::pickedStartNode, this, &ConnectedComponents::onPickedNewStartNode);
    connect(this, &IAlgorithm::visitedNode, this, &ConnectedComponents::onVisitedNode);
    connect(this, &IAlgorithm::analyzingNode, this, &ConnectedComponents::onAnalyzingNode);
    connect(this, &IAlgorithm::analyzedNode, this, &ConnectedComponents::onAnalyzedNode);

    m_pseudocodeForm.setHighlightPriority(CC_PSEUDOCODE_PRIORITY);
}

bool ConnectedComponents::step() {
    if (m_traversalContainer.empty()) {
        if (!m_currentConnectedComponent.empty()) {
            colorCurrentConnectedComponent();
            return true;
        }
    }

    return DepthFirstTotalTraversal::step();
}

void ConnectedComponents::start() {
    DepthFirstTotalTraversal::start(static_cast<NodeIndex_t>(
        Random::get().getSize(0, m_graph->getGraphManager().getNodesCount() - 1)));
}

void ConnectedComponents::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM CC;
(2)  BEGIN
(3)      U := N − {s}; V := {s}; W := ∅; p := 1; N′ = {s};
(4)      WHILE W ≠ N DO
(5)      BEGIN
(6)          WHILE V ≠ ∅ DO
(7)          BEGIN
(8)              se selectează cel mai nou nod x introdus în V;
(9)              IF există muchie [x, y] ∈ A și y ∈ U
(10)              THEN U := U − {y}; V := V ∪ {y}; N′ := N′ ∪ {y}
(11)              ELSE V := V − {x}; W := W ∪ {x};
(12)         END;
(13)         se tipăresc p și N′;
(14)         se selectează s ∈ U; U := U − {s}; V := {s}; p := p + 1;
(15)                       N′ := {s};
(16)     END;
(17) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1}, CC_PSEUDOCODE_PRIORITY);
}

void ConnectedComponents::onFinishedAlgorithm() {
    m_graph->getGraphManager().clearAlgorithmPaths();

    ITimedAlgorithm::onFinishedAlgorithm();
}

void ConnectedComponents::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList U, V, W, N;
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
    }

    for (const auto& connectedComponent : m_connectedComponents) {
        QStringList nodeStrings;
        for (const auto& node : connectedComponent) {
            nodeStrings << QString::number(node);
        }
        N << "{" + nodeStrings.join(", ") + "}";
    }

    infoLines << "U = {" + U.join(", ") + "}";
    infoLines << "V = {" + V.join(", ") + "}";
    infoLines << "W = {" + W.join(", ") + "}";
    infoLines << "";
    infoLines << "p = " + QString::number(m_connectedComponents.size());
    infoLines << "N' = {" + N.join(", ") + "}";

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void ConnectedComponents::colorCurrentConnectedComponent() {
    const auto algPathEntry = -static_cast<int64_t>(m_connectedComponents.size()) - 1;
    auto& graphManager = m_graph->getGraphManager();

    const auto color = Random::get().getColor();
    graphManager.setAlgorithmPathColor(algPathEntry, color);

    std::unordered_set<NodeIndex_t> componentSet(m_currentConnectedComponent.begin(),
                                                 m_currentConnectedComponent.end());
    for (const auto nodeIndex : m_currentConnectedComponent) {
        auto& node = graphManager.getNode(nodeIndex);

        node.setFillColor(color);
        graphManager.update(node.getBoundingRect());

        graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
            nodeIndex, [&](NodeIndex_t neighbour, CostType_t) {
                if (componentSet.contains(neighbour)) {
                    graphManager.addAlgorithmEdge(nodeIndex, neighbour, algPathEntry);
                }
            });
    }

    m_connectedComponents.push_back(std::move(m_currentConnectedComponent));
    m_pseudocodeForm.highlight({13}, CC_PSEUDOCODE_PRIORITY);
}

void ConnectedComponents::onPickedNewStartNode(NodeIndex_t startNode) {
    m_currentConnectedComponent.push_back(startNode);

    if (m_stepTimer.isActive()) {
        m_pseudocodeForm.highlight({14, 15}, CC_PSEUDOCODE_PRIORITY);
    }
}

void ConnectedComponents::onVisitedNode(NodeIndex_t node) {
    m_currentConnectedComponent.push_back(node);
    m_pseudocodeForm.highlight({10}, CC_PSEUDOCODE_PRIORITY);
}

void ConnectedComponents::onAnalyzingNode(NodeIndex_t node) {
    m_pseudocodeForm.highlight({8}, CC_PSEUDOCODE_PRIORITY);
}

void ConnectedComponents::onAnalyzedNode(NodeIndex_t node) {
    m_pseudocodeForm.highlight({11}, CC_PSEUDOCODE_PRIORITY);
}
