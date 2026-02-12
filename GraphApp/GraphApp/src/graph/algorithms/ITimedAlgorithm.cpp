#include <pch.h>

#include "ITimedAlgorithm.h"

#include "../form/playback_settings/PlaybackSettings.h"

ITimedAlgorithm::ITimedAlgorithm(Graph* graph) : IAlgorithm(graph) {
    connect(m_graph, &Graph::leftArrowPressed, this, &ITimedAlgorithm::onLeftArrowPressed);
    connect(m_graph, &Graph::rightArrowPressed, this, &ITimedAlgorithm::onRightArrowPressed);
    connect(m_graph, &Graph::spacePressed, this, &ITimedAlgorithm::onSpacePressed);

    markAllNodesUnvisited();
}

void ITimedAlgorithm::start() {
    PlaybackSettings settingsDialog;
    if (settingsDialog.exec() != QDialog::Accepted) {
        return cancelAlgorithm();
    }

    m_stepDelay = settingsDialog.runInstantly() ? 0 : settingsDialog.getStepDelay();
    m_iterationsPerStep = settingsDialog.getIterationsPerStep();

    if (!settingsDialog.showPseudocode()) {
        m_pseudocodeForm.close();
    }

    if (m_stepDelay == 0) {
        stepAll();
    } else {
        m_stepConnection =
            connect(&m_stepTimer, &QTimer::timeout, this, &ITimedAlgorithm::onTimerTimeout);

        if (!settingsDialog.startPaused()) {
            m_stepTimer.start(m_stepDelay);
        }
    }
}

void ITimedAlgorithm::stepAll() {
    m_stepDelay = 0;

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
            updateAlgorithmInfoText();
            emit finished();

            return;
        }
    }

    ++m_currentIteration;
    updateAlgorithmInfoText();
}

void ITimedAlgorithm::onLeftArrowPressed() {
    if (m_currentIteration == 0 || !m_stepConnection) {
        return;
    }

    const auto timerWasActive = m_stepTimer.isActive();

    --m_currentIteration;
    m_stepTimer.stop();

    markAllNodesUnvisited();
    resetForUndo();

    m_graph->getGraphManager().clearAlgorithmPaths();
    for (size_t i = 0; i < m_currentIteration; ++i) {
        for (int j = 0; j < m_iterationsPerStep; ++j) {
            step();
        }
    }

    updateAlgorithmInfoText();

    if (timerWasActive) {
        m_stepTimer.start();
    }

    m_graph->notifyLeftArrowPressed();
}

void ITimedAlgorithm::onRightArrowPressed() {
    if (!m_stepConnection) {
        return;
    }

    const auto timerWasActive = m_stepTimer.isActive();

    m_stepTimer.stop();
    onTimerTimeout();

    if (timerWasActive) {
        m_stepTimer.start(m_stepDelay);
    }

    m_graph->notifyRightArrowPressed();
}

void ITimedAlgorithm::onSpacePressed() {
    if (!m_stepConnection) {
        return;
    }

    if (m_stepTimer.isActive()) {
        m_graph->notifyAlgorithmPaused();
        m_stepTimer.stop();
    } else {
        m_graph->notifyAlgorithmResumed();
        m_stepTimer.start(m_stepDelay);
    }
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
