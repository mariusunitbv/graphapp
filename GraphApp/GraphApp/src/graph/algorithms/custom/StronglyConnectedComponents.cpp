#include <pch.h>

#include "StronglyConnectedComponents.h"

#include "../form/GraphApp.h"
#include "../random/Random.h"

StronglyConnectedComponents::StronglyConnectedComponents(Graph* graph)
    : ITimedAlgorithm(graph), m_depthTraversal(new DepthFirstTotalTraversal(graph)) {}

void StronglyConnectedComponents::start() {
    m_pseudocodeForm.highlight({3});

    connect(m_depthTraversal, &IAlgorithm::finished, this,
            &StronglyConnectedComponents::onFirstTraversalFinished);
    connect(m_depthTraversal, &IAlgorithm::aborted, this, &IAlgorithm::cancelAlgorithm);

    m_depthTraversal->start(static_cast<NodeIndex_t>(
        Random::get().getSize(0, m_graph->getGraphManager().getNodesCount() - 1)));
}

bool StronglyConnectedComponents::step() {
    const auto& components = m_invertedDepthTraversal->m_stronglyConnectedComponents;
    if (m_currentComponentIndex == components.size()) {
        return false;
    }

    colorStronglyConnectedComponent(components[m_currentComponentIndex++]);
    return true;
}

void StronglyConnectedComponents::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM CTC;
(2) BEGIN
(3)     PROCEDURA PTDF(G);
(4)     PROCEDURA INVERSARE(G);
(5)     PROCEDURA PTDF(G⁻¹);
(6)     PROCEDURA SEPARARE(G⁻¹);
(7) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void StronglyConnectedComponents::updateAlgorithmInfoText() const {}

void StronglyConnectedComponents::onFirstTraversalFinished() {
    m_pseudocodeForm.highlight({4});
    const auto invertedGraph = m_graph->getInvertedGraph();
    if (!invertedGraph) {
        throw std::runtime_error("Failed to get inverted graph for SCC.");
    }

    const auto window = new GraphApp(nullptr);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->setGraph(invertedGraph);
    window->show();

    m_invertedDepthTraversal = new CustomDepthFirstTraversal(invertedGraph);

    connect(m_invertedDepthTraversal, &IAlgorithm::finished, this,
            &StronglyConnectedComponents::onSecondTraversalFinished);

    connect(m_invertedDepthTraversal, &IAlgorithm::aborted, m_depthTraversal,
            &IAlgorithm::cancelAlgorithm);
    connect(m_invertedDepthTraversal, &IAlgorithm::aborted, this, &IAlgorithm::cancelAlgorithm);

    m_pseudocodeForm.highlight({5});
    m_invertedDepthTraversal->initializeForSCC(m_depthTraversal->m_nodesInfo);
    m_invertedDepthTraversal->start(m_invertedDepthTraversal->m_sortedNodesByAnalyzeTime[0]);
}

void StronglyConnectedComponents::onSecondTraversalFinished() {
    m_pseudocodeForm.highlight({6});

    m_graph->getGraphManager().clearAlgorithmPath(DepthFirstTraversal::ANALYZED_EDGE);

    ITimedAlgorithm::start();
}

void StronglyConnectedComponents::colorStronglyConnectedComponent(
    const std::vector<NodeIndex_t>& component) {
    const auto algPathEntry = -static_cast<int64_t>(m_currentComponentIndex) - 1;
    auto& graphManager = m_graph->getGraphManager();

    const auto color = Random::get().getColor();
    graphManager.setAlgorithmPathColor(algPathEntry, color);

    std::unordered_set<NodeIndex_t> componentSet(component.begin(), component.end());
    for (const auto nodeIndex : component) {
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
}

StronglyConnectedComponents::CustomDepthFirstTraversal::CustomDepthFirstTraversal(Graph* graph)
    : DepthFirstTraversal(graph) {
    connect(this, &IAlgorithm::analyzedNode, this, &CustomDepthFirstTraversal::onNodeAnalyzed);
}

bool StronglyConnectedComponents::CustomDepthFirstTraversal::step() {
    if (m_traversalContainer.empty()) {
        if (!m_currentStronglyConnectedComponent.empty()) {
            m_stronglyConnectedComponents.push_back(std::move(m_currentStronglyConnectedComponent));
        }

        return pickNewStartNode();
    }

    return DepthFirstTraversal::step();
}

void StronglyConnectedComponents::CustomDepthFirstTraversal::initializeForSCC(
    const std::vector<DFSInfo>& nodesInfo) {
    m_sortedNodesByAnalyzeTime.resize(nodesInfo.size());

    std::iota(m_sortedNodesByAnalyzeTime.begin(), m_sortedNodesByAnalyzeTime.end(), 0);
    std::sort(m_sortedNodesByAnalyzeTime.begin(), m_sortedNodesByAnalyzeTime.end(),
              [&](NodeIndex_t a, NodeIndex_t b) {
                  return nodesInfo[a].m_analyzeTime > nodesInfo[b].m_analyzeTime;
              });
}

bool StronglyConnectedComponents::CustomDepthFirstTraversal::pickNewStartNode() {
    for (const auto nodeIndex : m_sortedNodesByAnalyzeTime) {
        if (getNodeState(nodeIndex) != NodeData::State::UNVISITED) {
            continue;
        }

        setStartNode(nodeIndex);
        return true;
    }

    return false;
}

void StronglyConnectedComponents::CustomDepthFirstTraversal::onNodeAnalyzed(NodeIndex_t node) {
    m_currentStronglyConnectedComponent.push_back(node);
}
