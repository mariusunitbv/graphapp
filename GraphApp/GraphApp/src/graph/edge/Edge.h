#pragma once

#include "../node/Node.h"

class Edge : public QGraphicsObject {
    Q_OBJECT

   public:
    Edge(Node* a, Node* b, int cost);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;

    bool connectsNode(Node* node) const;

    void setColor(const QRgb c);
    void setProgress(float p);

    bool isLoop() const;

    void markForErasure();

   signals:
    void markedForErasure(Edge* edge);

   protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

   private:
    void connectNodeSignals(Node* node);

    void updatePosition();
    void onSelectedChanged(bool selected);

    void markUnvisited();
    void markVisited(Node* parent);
    void markAnalyzed();

    void markAvailableInPathFindingPath(Node* node);
    void markPath(Node* parent);

    void unmark();

    void drawSelfLoopEdge(QPainter* painter);
    void drawEdge(QPainter* painter);

    Node* m_startNode;
    Node* m_endNode;

    int m_cost;
    float m_progress = 0.;

    QLine m_line;
    QPolygon m_arrowHead;

    QRgb m_color{Node::k_defaultOutlineColor};

    static constexpr double k_arrowSize{15.};
};
