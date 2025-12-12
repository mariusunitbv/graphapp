#include <pch.h>

#include "Node.h"

NodeData::NodeData(size_t index, const QPoint& position) {
    setIndex(static_cast<NodeIndex_t>(index));
    setPosition(position);
}

QRect NodeData::getBoundingRect() const {
    return QRect{m_position.x() - k_radius, m_position.y() - k_radius, 2 * k_radius, 2 * k_radius};
}

void NodeData::setIndex(NodeIndex_t index) {
    m_index = index;
    m_label = QString::number(index);
}

NodeIndex_t NodeData::getIndex() const { return m_index; }

void NodeData::setFillColor(QRgb c) { m_fillColor = c; }

QColor NodeData::getFillColor() const { return QColor::fromRgba(m_fillColor); }

void NodeData::setLabel(const QString& label) { m_label = label; }

const QString& NodeData::getLabel() const { return m_label; }

void NodeData::setPosition(const QPoint& position) { m_position = position; }

QPoint NodeData::getPosition() const { return m_position; }

void NodeData::setSelected(bool selected, uint32_t selectTime) {
    m_selected = selected;
    m_selectTime = selectTime;
}

bool NodeData::isSelected() const { return m_selected; }

uint32_t NodeData::getSelectTime() const { return m_selectTime; }
