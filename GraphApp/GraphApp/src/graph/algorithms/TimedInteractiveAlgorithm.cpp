#include <pch.h>

#include "TimedInteractiveAlgorithm.h"

TimedInteractiveAlgorithm::TimedInteractiveAlgorithm(Graph* parent)
    : QObject(parent), m_graph(parent) {
    connect(parent, &Graph::spacePressed, this, &TimedInteractiveAlgorithm::onSpacePressed);

    connect(&m_stepTimer, &QTimer::timeout, this, [this]() {
        if (!stepOnce()) {
            m_stepTimer.stop();
            emit finished();
        } else {
            emit ticked();
        }
    });
}

void TimedInteractiveAlgorithm::showPseudocode() {
    const auto screenGeom = QGuiApplication::primaryScreen()->availableGeometry();

    int x = screenGeom.right() - m_pseudocodeForm.width();
    int y = screenGeom.bottom() - m_pseudocodeForm.height();

    m_pseudocodeForm.move(x, y);
}

void TimedInteractiveAlgorithm::setStepDelay(int stepDelay) { m_stepDelay = stepDelay; }

void TimedInteractiveAlgorithm::onSpacePressed() {
    if (!m_stepTimer.isActive()) {
        m_stepTimer.start(70);
    }

    m_stepTimer.setInterval(70);
    QTimer::singleShot(100, this, [this]() { m_stepTimer.setInterval(m_stepDelay); });
}
