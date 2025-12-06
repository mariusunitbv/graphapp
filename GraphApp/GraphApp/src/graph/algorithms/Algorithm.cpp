#include <pch.h>

#include "Algorithm.h"

Algorithm::Algorithm(Graph* parent) : QObject(parent), m_graph(parent) {
    connect(this, &Algorithm::finished, this, &Algorithm::onFinishedAlgorithm);
    connect(parent, &Graph::escapePressed, this, &Algorithm::onEscapePressed);
}

void Algorithm::showPseudocode() {
    const auto screenGeom = QGuiApplication::primaryScreen()->availableGeometry();

    int x = screenGeom.right() - m_pseudocodeForm.width();
    int y = screenGeom.bottom() - m_pseudocodeForm.height();

    m_pseudocodeForm.move(x, y);
}

void Algorithm::onEscapePressed() {
    m_pseudocodeForm.close();

    emit aborted();

    deleteLater();
}

void Algorithm::onFinishedAlgorithm() { m_pseudocodeForm.close(); }
