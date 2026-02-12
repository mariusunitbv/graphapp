#include <pch.h>

#include "KruskalMST.h"

KruskalMST::KruskalMST(Graph* graph) : ITimedAlgorithm(graph) {
    sortEdgesByCost();
    m_disjointSet = std::make_unique<DisjointSet>(m_graph->getGraphManager().getNodesCount());
    graph->getGraphManager().setAlgorithmPathColor(MST_EDGE, qRgb(60, 179, 113));
}

bool KruskalMST::step() {
    const auto nodeCount = m_graph->getGraphManager().getNodesCount();

    while (m_currentEdgeIndex < m_sortedEdges.size()) {
        const auto& [cost, u, v] = m_sortedEdges[m_currentEdgeIndex++];
        if (m_disjointSet->find(u) != m_disjointSet->find(v)) {
            m_disjointSet->unite(u, v);
            m_mstEdges.emplace_back(u, v);

            setNodeState(u, NodeData::State::VISITED);
            setNodeState(v, NodeData::State::VISITED);

            m_pseudocodeForm.highlight({6, 7});

            m_graph->getGraphManager().addAlgorithmEdge(u, v, MST_EDGE);
            return true;
        }
    }

    if (m_mstEdges.size() != nodeCount - 1) {
        QMessageBox::information(nullptr, "Kruskal MST",
                                 "The graph is disconnected. MST cannot be completed.",
                                 QMessageBox::Ok);
    }

    return false;
}

void KruskalMST::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM KRUSKAL;
(2) BEGIN
(3)     SORTARE(G);                     
(4)     A' := ∅;                         
(5)     FOR i := 0 TO m DO
(6)         IF aᵢ nu formează ciclu cu A' THEN
(7)             A' := A' ∪ {aᵢ};
(8)     END;
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void KruskalMST::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList A;
    for (const auto [node, neighbour] : m_mstEdges) {
        A << QString("[%1, %2]").arg(node).arg(neighbour);
    }

    infoLines << "A': {" + A.join(", ") + "}";

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void KruskalMST::resetForUndo() {
    m_disjointSet = std::make_unique<DisjointSet>(m_graph->getGraphManager().getNodesCount());
    m_mstEdges.clear();
    m_currentEdgeIndex = 0;
}

void KruskalMST::sortEdgesByCost() {
    const auto n = m_graph->getGraphManager().getNodesCount();
    m_sortedEdges.reserve(n * (n - 1) / 2);

    for (NodeIndex_t i = 0; i < n; ++i) {
        m_graph->getGraphManager().getGraphStorage()->forEachOutgoingEdge(
            i, [&](NodeIndex_t neighbour, CostType_t cost) {
                m_sortedEdges.emplace_back(cost, i, neighbour);
            });
    }

    std::sort(m_sortedEdges.begin(), m_sortedEdges.end());
}
