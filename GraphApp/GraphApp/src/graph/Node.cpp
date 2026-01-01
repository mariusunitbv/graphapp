#include <pch.h>

#include "Node.h"

NodeData::NodeData(size_t index, const QPoint& position) {
    setIndex(static_cast<NodeIndex_t>(index));
    setPosition(position);
}

QRect NodeData::getBoundingRect() const {
    return QRect{m_position.x() - k_radius, m_position.y() - k_radius, 2 * k_radius, 2 * k_radius};
}

void NodeData::setIndex(NodeIndex_t index) { m_index = index; }

NodeIndex_t NodeData::getIndex() const { return m_index; }

void NodeData::setFillColor(QRgb c) { m_fillColor = c; }

QColor NodeData::getFillColor() const { return QColor::fromRgba(m_fillColor); }

void NodeData::setLabel(const QString& label) { m_label = label; }

const QString& NodeData::getLabel() const {
    if (m_label.isEmpty()) {
        const_cast<NodeData*>(this)->m_label = QString::number(m_index);
    }

    return m_label;
}

void NodeData::setPosition(const QPoint& position) { m_position = position; }

QPoint NodeData::getPosition() const { return m_position; }

void NodeData::select(uint32_t selectOrder) { m_selectOrder = selectOrder; }

void NodeData::deselect() { m_selectOrder = -1; }

bool NodeData::isSelected() const { return m_selectOrder != -1; }

uint32_t NodeData::getSelectOrder() const { return m_selectOrder; }

void NodeData::setState(State state) {
    switch (state) {
        case State::NONE:
            break;
        case State::UNVISITED:
            setFillColor(qRgb(150, 150, 150));
            break;
        case State::VISITED:
            setFillColor(qRgb(70, 130, 180));
            break;
        case State::ANALYZING:
            setFillColor(qRgb(255, 165, 0));
            break;
        case State::ANALYZED:
            setFillColor(qRgb(60, 179, 113));
            break;
        case State::UNREACHABLE:
            setFillColor(qRgb(220, 20, 20));
            break;
        default:
            throw std::runtime_error("Invalid state assignment to NodeData");
    }

    m_state = state;
}

NodeData::State NodeData::getState() const { return m_state; }
