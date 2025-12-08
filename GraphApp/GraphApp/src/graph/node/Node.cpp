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
    painter->drawText(boundingRect(), Qt::AlignCenter, getLabel());
}

QPainterPath Node::shape() const {
    QPainterPath path;
    path.addEllipse(boundingRect());
    return path;
}

void Node::setFillColor(const QRgb c) {
    if (m_animationDisabled) {
        m_fill = c;
        update();
        return;
    }

    if (m_colorAnimation) {
        m_colorAnimation->stop();
    }

    QColor startColor = QColor::fromRgba(m_fill);
    QColor endColor = QColor::fromRgba(c);

    m_colorAnimation = new QVariantAnimation(this);
    m_colorAnimation->setStartValue(startColor);
    m_colorAnimation->setEndValue(endColor);
    m_colorAnimation->setDuration(250);
    m_colorAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    connect(m_colorAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_fill = v.value<QColor>().rgba();
        update();
    });

    m_colorAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

QRgb Node::getFillColor() const {
    if (m_colorAnimation) {
        return m_colorAnimation->endValue().value<QColor>().rgba();
    }

    return m_fill;
}

void Node::setOutlineColor(const QRgb c) {
    if (m_animationDisabled) {
        m_outline = c;
        update();
        return;
    }

    if (m_outlineColorAnimation) {
        m_outlineColorAnimation->stop();
    }

    QColor startColor = QColor::fromRgba(m_outline);
    QColor endColor = QColor::fromRgba(c);

    m_outlineColorAnimation = new QVariantAnimation(this);
    m_outlineColorAnimation->setStartValue(startColor);
    m_outlineColorAnimation->setEndValue(endColor);
    m_outlineColorAnimation->setDuration(250);
    m_outlineColorAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    connect(m_outlineColorAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& v) {
                m_outline = v.value<QColor>().rgba();
                update();
            });

    m_outlineColorAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Node::setOutlineWidth(float width) {
    if (m_animationDisabled) {
        m_outlineWidth = width;
        update();
        return;
    }

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
    if (m_animationDisabled) {
        QGraphicsObject::setOpacity(opacity);
        update();
        return;
    }

    if (m_opacityAnimation) {
        m_opacityAnimation->stop();
    }

    m_opacityAnimation = new QVariantAnimation(this);
    m_opacityAnimation->setStartValue(QGraphicsObject::opacity());
    m_opacityAnimation->setEndValue(opacity);
    m_opacityAnimation->setDuration(250);
    m_opacityAnimation->setEasingCurve(QEasingCurve::InOutBounce);

    connect(m_opacityAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        QGraphicsObject::setOpacity(v.toReal());
        update();
    });

    m_opacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Node::setScale(qreal scale) {
    if (m_animationDisabled) {
        QGraphicsObject::setScale(scale);
        update();
        return;
    }

    const auto anim = new QVariantAnimation(this);
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

void Node::setLabel(const QString& label) { m_label = label; }

QString Node::getLabel() const {
    if (m_label.isEmpty()) {
        return QString::number(m_index);
    }

    return m_label;
}

void Node::setAllNodesView(const std::vector<Node*>* allNodesView) {
    m_allNodesView = allNodesView;
}

void Node::markUnvisited() {
    setOutlineColor(qRgba(0, 0, 0, 40));

    m_state = State::UNVISITED;

    emit markedUnvisited();
}

void Node::markVisited() {
    setFillColor(k_defaultVisitedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::VISITED;

    emit markedVisited();
}

void Node::markVisitedButNotAnalyzedAnymore() {
    setFillColor(k_defaultVisitedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::VISITED;
}

void Node::markCurrentlyAnalyzed() {
    setFillColor(k_defaultCurrentlyAnalyzedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::CURRENTLY_ANALYZING;
}

void Node::markAnalyzed() {
    setFillColor(k_defaultAnalyzedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::ANALYZED;

    emit markedAnalyzed();
}

void Node::markPartOfConnectedComponent(QRgb c) {
    setFillColor(c);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::CONNECTED_COMPONENT;

    emit markedPartOfConnectedComponent(c);
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

void Node::markPath() {
    setFillColor(k_defaultAnalyzedColor);
    setOutlineColor(k_defaultOutlineColor);

    m_state = State::PATH;

    emit markedPath();
}

void Node::markForErasure() {
    if (m_animationDisabled) {
        emit markedForErasure(this);
        return;
    }

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
    if (isSelected()) {
        setOutlineColor(k_defaultSelectedOutlineColor);
    }

    emit unmarked();
}

void Node::setIndex(size_t index) { m_index = index; }

size_t Node::getIndex() const { return m_index; }

Node::State Node::getState() const { return m_state; }

void Node::setAnimationDisabled(bool disabled) { m_animationDisabled = disabled; }

bool Node::isAnimationDisabled() const { return m_animationDisabled; }

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

void Node::hoverEnterEvent(QGraphicsSceneHoverEvent*) {
    setCursor(Qt::OpenHandCursor);
    setOpacity(0.6);
}

void Node::hoverLeaveEvent(QGraphicsSceneHoverEvent*) { setOpacity(1); }

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
            if (scene() && !m_animationDisabled) {
                QGraphicsObject::setScale(0.01);
                setScale(1);
            }

            break;
        case ItemSelectedHasChanged:
            setOutlineWidth(value.toBool() ? 4.f : 1.5f);
            if (m_state == Node::State::NONE) {
                emit selectionChanged(value.toBool());
                setOutlineColor(value.toBool() ? k_defaultSelectedOutlineColor
                                               : k_defaultOutlineColor);
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

NodeData::NodeData(size_t index, const QPointF& position) : m_index(index) {
    setPosition(position);
    m_label = QString::number(index);
}

const QRect& NodeData::getBoundingRect() const { return m_boundingRect; }

void NodeData::setIndex(NodeIndex_t index) { m_index = index; }

NodeIndex_t NodeData::getIndex() const { return m_index; }

void NodeData::setPosition(const QPointF& position) {
    m_boundingRect.setRect(position.x() - k_radius, position.y() - k_radius, 2 * k_radius,
                           2 * k_radius);
}

QPoint NodeData::getPosition() const { return m_boundingRect.center(); }
