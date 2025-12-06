#include <pch.h>

#include "TimedInteractiveAlgorithm.h"

TimedInteractiveAlgorithm::TimedInteractiveAlgorithm(Graph* parent) : Algorithm(parent) {
    connect(parent, &Graph::spacePressed, this, &TimedInteractiveAlgorithm::onSpacePressed);

    m_stepConnection = connect(&m_stepTimer, &QTimer::timeout, this, [this]() {
        if (!stepOnce()) {
            emit finished();
        } else {
            emit ticked();
        }
    });
}

void TimedInteractiveAlgorithm::setStepDelay(int stepDelay) { m_stepDelay = stepDelay; }

void TimedInteractiveAlgorithm::onSpacePressed() {
    if (!m_stepTimer.isActive()) {
        m_stepTimer.start(70);
    }

    m_stepTimer.setInterval(70);
    QTimer::singleShot(100, this, [this]() { m_stepTimer.setInterval(m_stepDelay); });
}

void TimedInteractiveAlgorithm::onEscapePressed() {
    m_stepTimer.stop();

    Algorithm::onEscapePressed();
}

void TimedInteractiveAlgorithm::onFinishedAlgorithm() {
    if (m_stepConnection) {
        disconnect(m_stepConnection);
    }

    m_stepTimer.stop();
    Algorithm::onFinishedAlgorithm();
}
