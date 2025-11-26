#include <pch.h>

#include "algorithm_label.h"

AlgorithmLabel::AlgorithmLabel(QGraphicsScene* scene, QPointF pos,
                               std::function<void(QGraphicsSimpleTextItem*)> computeFunc)
    : m_scene(scene), m_label(new QGraphicsSimpleTextItem()), m_compute(std::move(computeFunc)) {
    m_label->setZValue(10);
    m_label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_label->setPos(pos);

    auto font = m_label->font();
    font.setPointSize(16);
    m_label->setFont(font);

    scene->addItem(m_label);
}

AlgorithmLabel::~AlgorithmLabel() {
    m_scene->removeItem(m_label);
    delete m_label;
}

void AlgorithmLabel::compute() const { m_compute(m_label); }
