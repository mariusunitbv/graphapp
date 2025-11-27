#include <pch.h>

#include "Node.h"

Node::Node(size_t index) : m_index{index} {
    setZValue(2);
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
}

Node::~Node() {}

QRectF Node::boundingRect() const {
    return QRectF(-k_fullRadius, -k_fullRadius, 2 * k_fullRadius, 2 * k_fullRadius);
}

void Node::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->setBrush(QColor::fromRgba(m_fill));

    painter->setPen(
        QPen{QColor::fromRgba(isSelected() ? k_defaultSelectedOutlineColor : m_outline), 1.5});
    painter->drawEllipse(-m_radius, -m_radius, 2 * m_radius, 2 * m_radius);

    painter->setPen(QColor::fromRgba(m_outline));
    if (m_radius >= 10) {
        painter->drawText(boundingRect(), Qt::AlignCenter, QString::number(m_index));
    }
}

QPainterPath Node::shape() const {
    QPainterPath path;
    path.addEllipse(boundingRect());
    return path;
}

int Node::type() const { return UserType + 1; }

void Node::setFillColor(const QRgb c) { m_fill = c; }

void Node::setOutlineColor(const QRgb c) { m_outline = c; }

void Node::setSelectedOutlineColor(const QRgb c) { /*m_selectedOutline = c;*/ }

void Node::markUnvisited() {
    setOutlineColor(qRgba(0, 0, 0, 40));

    emit markedUnvisited();
}

void Node::markVisited(Node* parent) {
    setFillColor(k_defaultVisitedColor);
    setOutlineColor(k_defaultOutlineColor);

    emit markedVisited(parent);
}

void Node::markVisitedButNotAnalyzedAnymore() {
    setFillColor(k_defaultVisitedColor);
    setOutlineColor(k_defaultOutlineColor);
}

void Node::markCurrentlyAnalyzed() {
    setFillColor(k_defaultCurrentlyAnalyzedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_internalState = Node::CURRENTLY_ANALYZED;
}

void Node::markAnalyzed(Node* parent) {
    setFillColor(k_defaultAnalyzedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_internalState = Node::ANALYZED;

    emit markedAnalyzed(parent);
}

void Node::markAvailableInPathFinding() {
    setFillColor(k_defaultFillColor);
    setOutlineColor(k_defaultOutlineColor);

    emit markedAvailableInPathFinding(this);
}

void Node::markPath(Node* parent) {
    setFillColor(k_defaultAnalyzedColor);
    setOutlineColor(k_defaultOutlineColor);

    emit markedPath(parent);
}

void Node::unmark() {
    setFillColor(k_defaultFillColor);
    setOutlineColor(k_defaultOutlineColor);

    m_internalState = Node::NONE;

    emit unmarked();
}

void Node::setIndex(size_t index) { m_index = index; }

size_t Node::getIndex() const { return m_index; }

void Node::setRadius(double radius) {
    m_radius = radius;
    update();
}

double Node::getRadius() const { return m_radius; }

Node::InternalState Node::getInternalState() const { return m_internalState; }

void Node::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);
    }

    QGraphicsObject::mousePressEvent(event);
}

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    setCursor(Qt::OpenHandCursor);
    QGraphicsObject::mouseReleaseEvent(event);
}

QVariant Node::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change != ItemPositionChange || !scene()) {
        return QGraphicsObject::itemChange(change, value);
    }

    auto newPos = value.toPointF();

    const auto& sceneRect = scene()->sceneRect();
    const auto x = qBound(sceneRect.left() + boundingRect().width() / 2, newPos.x(),
                          sceneRect.right() - boundingRect().width() / 2);
    const auto y = qBound(sceneRect.top() + boundingRect().height() / 2, newPos.y(),
                          sceneRect.bottom() - boundingRect().height() / 2);
    newPos = QPointF(x, y);

    QPainterPath circlePath;
    circlePath.addEllipse(boundingRect().translated(newPos));

    const auto collisions = scene()->items(circlePath, Qt::IntersectsItemShape);
    const bool collides = [&]() {
        for (auto item : collisions) {
            if (item->type() == NodeType && item != this) {
                return true;
            }
        }

        return false;
    }();

    if (collides) {
        return pos();
    }

    emit positionChanged();
    return newPos;
}
