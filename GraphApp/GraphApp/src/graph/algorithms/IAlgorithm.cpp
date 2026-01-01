#include <pch.h>

#include "IAlgorithm.h"

IAlgorithm::IAlgorithm(Graph* graph) : QObject(graph), m_graph(graph) {
    m_graph->getGraphManager().registerAlgorithm(this);

    connect(this, &IAlgorithm::finished, this, &IAlgorithm::onFinishedAlgorithm);
}

IAlgorithm::~IAlgorithm() { m_graph->getGraphManager().unregisterAlgorithm(this); }

void IAlgorithm::setNodeState(NodeIndex_t nodeIndex, NodeData::State state) {
    auto& graphManager = m_graph->getGraphManager();
    auto& node = graphManager.getNode(nodeIndex);

    node.setState(state);
    graphManager.update(node.getBoundingRect());
}

NodeData::State IAlgorithm::getNodeState(NodeIndex_t nodeIndex) const {
    return m_graph->getGraphManager().getNode(nodeIndex).getState();
}

void IAlgorithm::cancelAlgorithm() {
    if (m_cancelRequested) {
        return;
    }

    m_cancelRequested = true;
    m_pseudocodeForm.close();

    emit aborted();

    deleteLater();
}

void IAlgorithm::onFinishedAlgorithm() { m_pseudocodeForm.close(); }

void IAlgorithm::showPseudocodeForm() {
    const auto screenGeom = QGuiApplication::primaryScreen()->availableGeometry();

    int x = screenGeom.right() - m_pseudocodeForm.width();
    int y = screenGeom.bottom() - m_pseudocodeForm.height();

    m_pseudocodeForm.show();
    m_pseudocodeForm.move(x, y);
}
