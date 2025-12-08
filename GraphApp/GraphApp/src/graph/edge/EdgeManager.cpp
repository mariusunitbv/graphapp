#include <pch.h>

#include "EdgeManager.h"

EdgeManager::EdgeManager() {
    connect(&m_sceneUpdateTimer, &QTimer::timeout, [this]() {
        QGraphicsView *view = scene()->views().first();
        QRectF sceneRect = view->mapToScene(view->viewport()->rect()).boundingRect();
        if (m_sceneRect != sceneRect) {
            m_sceneRect = sceneRect.toRect();
            update(m_sceneRect);
        }
    });
}

void EdgeManager::setSceneDimensions(qreal width, qreal height) {
    m_boundingRect.setCoords(0, 0, width, height);
    m_sceneUpdateTimer.start(200);
}

QRectF EdgeManager::boundingRect() const { return m_boundingRect; }

void EdgeManager::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) {}

bool EdgeManager::isVisibleInScene(const QRect &rect) const { return m_sceneRect.intersects(rect); }
