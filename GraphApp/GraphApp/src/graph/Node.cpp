#include <pch.h>

#include "Node.h"

NodeData::NodeData(size_t index, const QPoint& position) {
    setIndex(static_cast<NodeIndex_t>(index));
    setPosition(position);
}

const QRect& NodeData::getBoundingRect() const { return m_boundingRect; }

void NodeData::setIndex(NodeIndex_t index) {
    m_index = index;
    m_label = QString::number(index);
}

NodeIndex_t NodeData::getIndex() const { return m_index; }

void NodeData::setFillColor(QRgb c) { m_fillColor = c; }

QColor NodeData::getFillColor() const { return QColor::fromRgba(m_fillColor); }

void NodeData::setPosition(const QPoint& position) {
    m_boundingRect.setRect(position.x() - k_radius, position.y() - k_radius, 2 * k_radius,
                           2 * k_radius);
}

QPoint NodeData::getPosition() const { return m_boundingRect.center(); }
