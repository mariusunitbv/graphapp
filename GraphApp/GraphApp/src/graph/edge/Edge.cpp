#include <pch.h>

#include "Edge.h"

Edge::Edge(Node* a, Node* b, int cost) : m_startNode(a), m_endNode(b), m_cost(cost) {
    setZValue(-1);

    connectNodeSignals(a);
    if (a != b) {
        connectNodeSignals(b);
    }

    connect(a, &Node::selectionChanged, this, &Edge::onSelectedChanged);

    updatePosition();
}

QRectF Edge::boundingRect() const {
    if (isLoop()) {
        const auto center = m_startNode->pos();
        const auto r = m_startNode->k_radius;

        return QRectF(center.x() + r * 0.3, center.y() - r * 1.25, r, r);
    }

    return QRectF(m_line.p1(), m_line.p2()).normalized();
}

void Edge::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    if (isLoop()) {
        drawSelfLoopEdge(painter);
    } else {
        drawEdge(painter);
    }
}

bool Edge::connectsNode(Node* node) const { return m_startNode == node || m_endNode == node; }

void Edge::setColor(const QRgb c) {
    QColor startColor = QColor::fromRgba(m_color);
    QColor endColor = QColor::fromRgba(c);

    auto anim = new QVariantAnimation(this);
    anim->setStartValue(startColor);
    anim->setEndValue(endColor);
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::InOutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_color = v.value<QColor>().rgba();
        update();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Edge::setProgress(float p) {
    auto anim = new QVariantAnimation(this);
    anim->setStartValue(m_progress);
    anim->setEndValue(p);
    anim->setDuration(500);
    anim->setEasingCurve(QEasingCurve::InOutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_progress = v.toReal();
        update();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

bool Edge::isLoop() const { return m_startNode == m_endNode; }

void Edge::markForErasure() {
    disconnect(m_startNode);
    disconnect(m_endNode);

    auto anim = new QVariantAnimation(this);
    anim->setStartValue(m_progress);
    anim->setEndValue(0.05f);
    anim->setDuration(150);
    anim->setEasingCurve(QEasingCurve::InOutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_progress = v.toReal();
        update();
    });

    connect(anim, &QVariantAnimation::finished, this, [this]() { emit markedForErasure(this); });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

QVariant Edge::itemChange(GraphicsItemChange change, const QVariant& value) {
    switch (change) {
        case ItemSceneHasChanged:
            if (scene()) {
                setProgress(1.);
            }

            break;
    }

    return QGraphicsObject::itemChange(change, value);
}

void Edge::connectNodeSignals(Node* node) {
    connect(node, &Node::changedPosition, this, &Edge::updatePosition);

    connect(node, &Node::markedUnvisited, this, &Edge::markUnvisited);
    connect(node, &Node::markedVisited, this, &Edge::markVisited);
    connect(node, &Node::markedAnalyzed, this, &Edge::markAnalyzed);

    connect(node, &Node::markedAvailableInPathFinding, this, &Edge::markAvailableInPathFindingPath);
    connect(node, &Node::markedPath, this, &Edge::markPath);

    connect(node, &Node::unmarked, this, &Edge::unmark);
}

void Edge::updatePosition() {
    prepareGeometryChange();
    m_arrowHead.clear();

    const auto srcCenter = QPoint(m_startNode->pos().x(), m_startNode->pos().y());
    const auto targetCenter = QPoint(m_endNode->pos().x(), m_endNode->pos().y());

    const auto direction = targetCenter - srcCenter;
    const auto distanceBetweenNodes = std::hypot(direction.x(), direction.y());

    if (distanceBetweenNodes < 0.0001) {
        return;
    }

    const auto radiusOffset = direction * (m_startNode->k_radius / distanceBetweenNodes);

    const auto lineStart = srcCenter + radiusOffset;
    const auto lineEnd = targetCenter - radiusOffset;

    m_line.setPoints(lineStart, lineEnd);

    const auto angle = std::atan2(-direction.y(), direction.x());
    const auto arrowP1 =
        lineEnd - QPoint(sin(angle + M_PI / 3) * k_arrowSize, cos(angle + M_PI / 3) * k_arrowSize);
    const auto arrowP2 = lineEnd - QPoint(sin(angle + M_PI - M_PI / 3) * k_arrowSize,
                                          cos(angle + M_PI - M_PI / 3) * k_arrowSize);
    m_arrowHead << lineEnd << arrowP1 << arrowP2;
}

void Edge::onSelectedChanged(bool selected) {
    setColor(selected ? Node::k_defaultAnalyzedColor : Node::k_defaultOutlineColor);
    setZValue(selected ? 1 : -1);
}

void Edge::markUnvisited() { setColor(Node::k_defaultUnvisitedOutlineColor); }

void Edge::markVisited(Node* parent) {
    if (parent != m_startNode) {
        return;
    }

    setColor(Node::k_defaultCurrentlyAnalyzedColor);
}

void Edge::markAnalyzed() {
    if (m_startNode->getState() != Node::State::ANALYZED) {
        return;
    }

    setColor(Node::k_defaultAnalyzedColor);
}

void Edge::markAvailableInPathFindingPath(Node* node) {
    if (node != m_startNode) {
        return;
    }

    setColor(Node::k_defaultOutlineColor);
}

void Edge::markPath(Node* parent) {
    if (parent != m_endNode) {
        return;
    }

    setColor(Node::k_defaultAnalyzedColor);
}

void Edge::unmark() { setColor(Node::k_defaultOutlineColor); }

void Edge::drawSelfLoopEdge(QPainter* painter) {
    painter->setPen(m_color);

    int spanAngle = static_cast<int>(360 * 16 * m_progress);
    painter->drawArc(boundingRect(), 200 * 16, spanAngle);

    if (m_cost > 0 && m_progress == 1) {
        painter->drawText(boundingRect(), Qt::AlignCenter, QString::number(m_cost));
    }
}

void Edge::drawEdge(QPainter* painter) {
    const auto& p1 = m_line.p1();
    const auto& p2 = m_line.p2();

    const auto current = p1 + (p2 - p1) * m_progress;

    painter->setPen(m_color);
    painter->setBrush(m_color);
    painter->drawLine(p1, current);

    if (m_progress == 1) {
        painter->drawPolygon(m_arrowHead);
        if (m_cost > 0) {
            const auto srcCenter = m_startNode->pos();
            const auto targetCenter = m_endNode->pos();

            const auto direction = targetCenter - srcCenter;
            const auto distanceBetweenNodes = std::hypot(direction.x(), direction.y());
            const auto radiusOffset = direction * (m_startNode->k_radius / distanceBetweenNodes);

            const auto lineStart = srcCenter + radiusOffset;
            const auto lineEnd = targetCenter - radiusOffset;
            const auto midPoint = (lineStart + lineEnd) * 0.5;

            const auto angle = std::atan2(-direction.y(), direction.x());
            const auto angleDeg = [&]() {
                const auto rv = -angle * 180.0 / M_PI;
                if (rv < -90 || rv > 90) {
                    return rv + 180.0;
                }

                return rv;
            }();

            painter->save();
            painter->translate(midPoint);
            painter->rotate(angleDeg);

            const auto costText = QString::number(m_cost);
            QFontMetrics fm(painter->font());
            QRect textRect = fm.boundingRect(costText);
            textRect.moveCenter(QPoint(0, -10));

            painter->drawText(textRect, Qt::AlignCenter, costText);

            painter->restore();
        }
    }
}
