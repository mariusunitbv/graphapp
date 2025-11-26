#include <pch.h>

#include "Edge.h"

Edge::Edge(Node* a, Node* b, int cost) : m_startNode(a), m_endNode(b), m_cost(cost) {
    setZValue(-1);

    connectNodeSignals(a);
    if (a != b) {
        connectNodeSignals(b);
    }

    updatePosition();
}

Edge::~Edge() {}

QRectF Edge::boundingRect() const {
    if (isLoop()) {
        const auto center = m_startNode->pos();
        const auto r = m_startNode->getRadius();

        return QRectF(center.x() + r * 0.3, center.y() - r * 1.25, r, r);
    }

    return QRectF(m_line.p1(), m_line.p2()).normalized();
}

void Edge::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    if (isLoop()) {
        drawSelfLoopEdge(painter);
    } else {
        drawEdge(painter);
    }
}

int Edge::type() const { return UserType + 2; }

bool Edge::connectsNode(Node* node) const { return m_startNode == node || m_endNode == node; }

void Edge::setColor(const QColor& c) { m_color = c; }

qreal Edge::getProgress() const { return m_progress; }

void Edge::setProgress(qreal p) {
    m_progress = p;
    update();
}

bool Edge::isLoop() const { return m_startNode == m_endNode; }

void Edge::connectNodeSignals(Node* node) {
    connect(node, &Node::positionChanged, this, &Edge::updatePosition);

    connect(node, &Node::markedUnvisited, this, &Edge::markUnvisited);
    connect(node, &Node::markedVisited, this, &Edge::markVisited);
    connect(node, &Node::markedAnalyzed, this, &Edge::markAnalyzed);

    connect(node, &Node::markedAvailableInPathFinding, this, &Edge::markAvailableInPathFindingPath);
    connect(node, &Node::markedPath, this, &Edge::markPath);

    connect(node, &Node::unmarked, this, &Edge::unmark);
}

void Edge::updatePosition() {
    const auto srcCenter = m_startNode->pos();
    const auto targetCenter = m_endNode->pos();

    const auto direction = targetCenter - srcCenter;
    const auto distanceBetweenNodes = std::hypot(direction.x(), direction.y());
    if (distanceBetweenNodes < 0.0001) {
        m_arrowHead.clear();
        return;
    }

    const auto radiusOffset = direction * (m_startNode->getRadius() / distanceBetweenNodes);

    const auto lineStart = srcCenter + radiusOffset;
    const auto lineEnd = targetCenter - radiusOffset;

    m_line.setPoints(lineStart, lineEnd);

    const auto angle = std::atan2(-direction.y(), direction.x());
    const auto arrowP1 =
        lineEnd - QPointF(sin(angle + M_PI / 3) * k_arrowSize, cos(angle + M_PI / 3) * k_arrowSize);
    const auto arrowP2 = lineEnd - QPointF(sin(angle + M_PI - M_PI / 3) * k_arrowSize,
                                           cos(angle + M_PI - M_PI / 3) * k_arrowSize);
    m_arrowHead = QPolygonF({lineEnd, arrowP1, arrowP2});
}

void Edge::markUnvisited() { setColor(Qt::lightGray); }

void Edge::markVisited(Node* parent) {
    if (parent != m_startNode) {
        return;
    }

    setColor(Qt::yellow);
}

void Edge::markAnalyzed(Node* parent) {
    if (parent != m_startNode) {
        return;
    }

    setColor(Qt::green);
}

void Edge::markAvailableInPathFindingPath(Node* node) {
    if (node != m_startNode) {
        return;
    }

    setColor(Qt::black);
}

void Edge::markPath(Node* parent) {
    if (parent != m_endNode) {
        return;
    }

    setColor(Qt::green);
}

void Edge::unmark() { setColor(Qt::black); }

void Edge::drawSelfLoopEdge(QPainter* painter) {
    painter->setPen(m_startNode->isSelected() ? m_selectedColor : m_color);

    int spanAngle = static_cast<int>(360 * 16 * m_progress);
    painter->drawArc(boundingRect(), 200 * 16, spanAngle);
}

void Edge::drawEdge(QPainter* painter) {
    const auto& p1 = m_line.p1();
    const auto& p2 = m_line.p2();

    const auto current = p1 + (p2 - p1) * m_progress;

    setZValue(m_startNode->isSelected() ? 1 : -1);

    painter->setPen(QPen{m_startNode->isSelected() ? m_selectedColor : m_color});
    painter->drawLine(p1, current);
    painter->setBrush(m_startNode->isSelected() ? m_selectedColor : m_color);

    if (m_progress == 1) {
        painter->drawPolygon(m_arrowHead);
        if (m_cost > 0) {
            const auto srcCenter = m_startNode->pos();
            const auto targetCenter = m_endNode->pos();

            const auto direction = targetCenter - srcCenter;
            const auto distanceBetweenNodes = std::hypot(direction.x(), direction.y());
            const auto radiusOffset = direction * (m_startNode->getRadius() / distanceBetweenNodes);

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
