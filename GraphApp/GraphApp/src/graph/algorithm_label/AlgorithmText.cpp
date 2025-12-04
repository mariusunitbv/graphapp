#include <pch.h>

#include "AlgorithmText.h"

#include "../Graph.h"

struct AlgorithmTextComparator {
    bool operator()(AlgorithmText* a, AlgorithmText* b) const {
        return a->getOrder() < b->getOrder();
    }
};

static std::set<AlgorithmText*, AlgorithmTextComparator> g_allAlgorithmTexts;

AlgorithmText::AlgorithmText(Graph* graph, int order, int additionalSpacing)
    : QGraphicsTextItem(), m_graph(graph), m_order(order), m_additionalSpacing(additionalSpacing) {
    setZValue(10);
    setFlag(ItemIgnoresTransformations);
    setDefaultTextColor(Qt::black);

    auto textFont = font();
    textFont.setPointSize(14);
    setFont(textFont);

    connect(m_graph, &Graph::zoomChanged, this, &AlgorithmText::repositionAll);
    connect(m_graph, &Graph::movedGraph, this, &AlgorithmText::repositionAll);

    m_graph->scene()->addItem(this);
    g_allAlgorithmTexts.emplace(this);
}

AlgorithmText::~AlgorithmText() {
    m_graph->scene()->removeItem(this);
    g_allAlgorithmTexts.erase(this);
}

int AlgorithmText::getOrder() const { return m_order; }

void AlgorithmText::setComputeFunction(std::function<void(AlgorithmText*)> computeFunction) {
    m_computeFunction = std::move(computeFunction);
}

void AlgorithmText::compute() {
    repositionAll();
    m_computeFunction(this);
}

void AlgorithmText::repositionAll() {
    const auto sceneOffset10px =
        m_graph->mapToScene(QPoint(0, 10)).y() - m_graph->mapToScene(QPoint(0, 0)).y();

    auto currentPos = m_graph->mapToScene(QPoint(10, 10));
    for (AlgorithmText* text : g_allAlgorithmTexts) {
        text->setTextWidth(m_graph->width() - 20);
        text->setPos(currentPos);

        const auto sceneOffsetAdditional =
            m_graph->mapToScene(QPoint(0, text->m_additionalSpacing)).y() -
            m_graph->mapToScene(QPoint(0, 0)).y();
        const auto localH = text->boundingRect().height();
        const auto sceneH =
            m_graph->mapToScene(QPoint(0, localH)).y() - m_graph->mapToScene(QPoint(0, 0)).y();
        currentPos.setY(currentPos.y() + sceneH + sceneOffset10px + sceneOffsetAdditional);
    }
}
