#include <pch.h>

#include "ITimedAlgorithm.h"

ITimedAlgorithm::ITimedAlgorithm(Graph* graph) : IAlgorithm(graph) {
    connect(m_graph, &Graph::spacePressed, this, &ITimedAlgorithm::onSpacePressed);
    m_stepConnection =
        connect(&m_stepTimer, &QTimer::timeout, this, &ITimedAlgorithm::onTimerTimeout);

    markAllNodesUnvisited();
}

void ITimedAlgorithm::start() {
    bool ok = false;

    m_stepDelay = QInputDialog::getInt(
        nullptr, "Step Delay", "Enter step delay in milliseconds:", 1000, 0, 30000, 500, &ok);
    if (!ok) {
        return cancelAlgorithm();
    }

    if (m_stepDelay > 0) {
        m_iterationsPerStep =
            QInputDialog::getInt(nullptr, "Iterations Per Step",
                                 "Enter number of iterations per step:", 1, 1, 100000, 1, &ok);
        if (!ok) {
            return cancelAlgorithm();
        }
    }

    if (m_stepDelay < PseudocodeForm::k_animationDurationMs || m_iterationsPerStep > 1) {
        m_pseudocodeForm.close();
    }

    if (m_stepDelay == 0) {
        stepAll();
    } else {
        m_stepTimer.start(m_stepDelay);
    }
}

void ITimedAlgorithm::stepAll() {
    while (step()) {
    }

    emit finished();

    updateAlgorithmInfoText();
}

void ITimedAlgorithm::markAllNodesUnvisited() {
    const auto nodeCount = static_cast<NodeIndex_t>(m_graph->getGraphManager().getNodesCount());
    for (NodeIndex_t i = 0; i < nodeCount; ++i) {
        setNodeState(i, NodeData::State::UNVISITED);
    }
}

void ITimedAlgorithm::unmarkAllNodes() {
    const auto nodeCount = static_cast<NodeIndex_t>(m_graph->getGraphManager().getNodesCount());
    for (NodeIndex_t i = 0; i < nodeCount; ++i) {
        m_graph->getGraphManager().getNode(i).setFillColor(m_graph->getDefaultNodeColor());
        setNodeState(i, NodeData::State::NONE);
    }
}

void ITimedAlgorithm::onTimerTimeout() {
    if (m_cancelRequested) {
        return;
    }

    for (int i = 0; i < m_iterationsPerStep; ++i) {
        if (!step()) {
            m_stepTimer.stop();
            updateAlgorithmInfoText();

            emit finished();

            return;
        }
    }

    updateAlgorithmInfoText();
}

void ITimedAlgorithm::onSpacePressed() {
    if (!m_stepTimer.isActive()) {
        return;
    }

    m_stepTimer.stop();
    onTimerTimeout();
    m_stepTimer.start(m_stepDelay);
}

void ITimedAlgorithm::cancelAlgorithm() {
    m_stepTimer.stop();

    m_graph->getGraphManager().clearAlgorithmPaths();
    unmarkAllNodes();

    IAlgorithm::cancelAlgorithm();
}

void ITimedAlgorithm::onFinishedAlgorithm() {
    if (m_stepConnection) {
        disconnect(m_stepConnection);
    }

    m_stepTimer.stop();

    IAlgorithm::onFinishedAlgorithm();
}
