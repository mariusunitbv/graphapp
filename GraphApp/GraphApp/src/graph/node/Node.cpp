#include <pch.h>

#include "Node.h"

Node::Node(size_t index) : m_index{index} {
    setZValue(2);
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);

    setAcceptHoverEvents(true);
}

QRectF Node::boundingRect() const {
    return QRectF(-k_radius, -k_radius, 2 * k_radius, 2 * k_radius);
}

void Node::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->setBrush(m_fill);
    painter->setPen(QPen{QColor::fromRgba(m_outline), m_outlineWidth});
    painter->drawEllipse(boundingRect());
    painter->drawText(boundingRect(), Qt::AlignCenter, QString::number(m_index));
}

QPainterPath Node::shape() const {
    QPainterPath path;
    path.addEllipse(boundingRect());
    return path;
}

void Node::setFillColor(const QRgb c) {
    QColor startColor = QColor::fromRgba(m_fill);
    QColor endColor = QColor::fromRgba(c);

    auto anim = new QVariantAnimation(this);
    anim->setStartValue(startColor);
    anim->setEndValue(endColor);
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::InOutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_fill = v.value<QColor>().rgba();
        update();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Node::setOutlineColor(const QRgb c, bool ignoreSelection) {
    if (!ignoreSelection && isSelected()) {
        m_outlineBackup = c;
        return;
    }

    QColor startColor = QColor::fromRgba(m_outline);
    QColor endColor = QColor::fromRgba(c);

    auto anim = new QVariantAnimation(this);
    anim->setStartValue(startColor);
    anim->setEndValue(endColor);
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::InOutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_outline = v.value<QColor>().rgba();
        update();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Node::setOutlineWidth(float width) {
    auto anim = new QVariantAnimation(this);
    anim->setStartValue(m_outlineWidth);
    anim->setEndValue(width);
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::InOutBounce);

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_outlineWidth = v.toReal();
        update();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Node::setOpacity(qreal opacity) {
    auto anim = new QVariantAnimation(this);
    anim->setStartValue(QGraphicsObject::opacity());
    anim->setEndValue(opacity);
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::InOutBounce);

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        QGraphicsObject::setOpacity(v.toReal());
        update();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Node::setScale(qreal scale) {
    auto anim = new QVariantAnimation(this);
    anim->setStartValue(QGraphicsObject::scale());
    anim->setEndValue(scale);
    anim->setDuration(500);
    anim->setEasingCurve(QEasingCurve::OutBack);

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        QGraphicsObject::setScale(v.toReal());
        update();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Node::setAllNodesView(const std::vector<Node*>* allNodesView) {
    m_allNodesView = allNodesView;
}

void Node::markUnvisited() {
    setOutlineColor(qRgba(0, 0, 0, 40));

    m_state = State::UNVISITED;

    emit markedUnvisited();
}

void Node::markVisited(Node* parent) {
    setFillColor(k_defaultVisitedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::VISITED;

    emit markedVisited(parent);
}

void Node::markVisitedButNotAnalyzedAnymore() {
    setFillColor(k_defaultVisitedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::VISITED;
}

void Node::markCurrentlyAnalyzed() {
    setFillColor(k_defaultCurrentlyAnalyzedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::CURRENTLY_ANALYZED;
}

void Node::markAnalyzed() {
    setFillColor(k_defaultAnalyzedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::ANALYZED;

    emit markedAnalyzed(this);
}

void Node::markAvailableInPathFinding() {
    setFillColor(k_defaultFillColor);
    setOutlineColor(k_defaultOutlineColor);

    emit markedAvailableInPathFinding(this);
}

void Node::markUnreachable() {
    setFillColor(k_defaultUnreachableColor);
    setOutlineColor(k_defaultOutlineColor);
}

void Node::markPath(Node* parent) {
    setFillColor(k_defaultAnalyzedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::PATH;

    emit markedPath(parent);
}

void Node::markForErasure() {
    auto anim = new QVariantAnimation(this);
    anim->setStartValue(scale());
    anim->setEndValue(0.01);
    anim->setDuration(400);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        QGraphicsObject::setScale(v.toReal());
        update();
    });

    connect(anim, &QVariantAnimation::finished, this, [this]() { emit markedForErasure(this); });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Node::unmark() {
    setFillColor(k_defaultFillColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::NONE;

    emit unmarked();
}

void Node::setIndex(size_t index) { m_index = index; }

size_t Node::getIndex() const { return m_index; }

Node::State Node::getState() const { return m_state; }

void Node::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);

        setOpacity(0.4);
        setScale(1.2);
        QTimer::singleShot(200, this, [this]() { setScale(1.); });
    }

    QGraphicsObject::mousePressEvent(event);
}

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setOpacity(1);
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

void Node::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    setCursor(Qt::OpenHandCursor);
    setOpacity(0.6);
}

void Node::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) { setOpacity(1); }

QVariant Node::itemChange(GraphicsItemChange change, const QVariant& value) {
    switch (change) {
        case ItemPositionChange:
            if (scene()) {
                return getGoodPositionWhenMoving(value.toPointF());
            }

            break;
        case ItemPositionHasChanged:
            emit changedPosition();

            break;
        case ItemSceneHasChanged:
            if (scene()) {
                QGraphicsObject::setScale(0.1f);
                setScale(1.0f);
            }

            break;
        case ItemSelectedHasChanged:
            if (value.toBool()) {
                m_outlineBackup = m_outline;
                setOutlineColor(k_defaultSelectedOutlineColor, true);
                setOutlineWidth(4.f);
            } else {
                setOutlineColor(m_outlineBackup);
                setOutlineWidth(1.5f);
            }

            if (m_state == Node::State::NONE) {
                emit selectionChanged(value.toBool());
            }

            break;
    }

    return QGraphicsObject::itemChange(change, value);
}

QPointF Node::getGoodPositionWhenMoving(const QPointF& desiredPos) {
    const auto& sceneRect = scene()->sceneRect();
    const auto x = qBound(sceneRect.left() + boundingRect().width() / 2, desiredPos.x(),
                          sceneRect.right() - boundingRect().width() / 2);
    const auto y = qBound(sceneRect.top() + boundingRect().height() / 2, desiredPos.y(),
                          sceneRect.bottom() - boundingRect().height() / 2);
    const auto newPos = QPointF(x, y);

    for (Node* node : *m_allNodesView) {
        if (node == this) {
            continue;
        }

        const auto delta = newPos - node->pos();
        const auto distSq = delta.x() * delta.x() + delta.y() * delta.y();

        static constexpr auto minDist = 2. * k_radius;
        if (distSq < (minDist * minDist)) {
            return pos();
        }
    }

    return newPos;
}
