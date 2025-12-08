#pragma once

#include "../node/Node.h"

class Edge : public QGraphicsObject {
    Q_OBJECT

   public:
    Edge(Node* a, Node* b, int cost);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;

    bool connectsNode(Node* node) const;

    Node* getStartNode() const;
    Node* getEndNode() const;
    int getCost() const;

    void setColor(const QRgb c);
    void setProgress(float p);
    void setUnorientedEdge(bool unoriented);

    bool isLoop() const;

    void markForErasure();
    bool areAnimationsDisabled() const;

   signals:
    void markedForErasure(Edge* edge);

   protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

   private:
    void connectNodeSignals(Node* node);

    void updatePosition();
    void onSelectedChanged(bool selected);

    void markUnvisited();
    void markVisited();
    void markAnalyzed();
    void markPartOfConnectedComponent(QRgb c);

    void markAvailableInPathFindingPath(Node* node);
    void markPath();

    void unmark();

    void drawSelfLoopEdge(QPainter* painter);
    void drawEdge(QPainter* painter);

    Node* m_startNode;
    Node* m_endNode;

    int m_cost;
    float m_progress{0.};

    QLine m_line;
    QPolygon m_arrowHead;

    QRgb m_color{Node::k_defaultOutlineColor};
    bool m_unorientedEdge{false};

    QPointer<QVariantAnimation> m_colorAnimation;

    static constexpr double k_arrowSize{15.};
};

class EdgeData {
   public:
    friend class NodeManager;

    EdgeData(NodeIndex_t start, NodeIndex_t end, int cost);

   private:
    NodeIndex_t m_startNode;
    NodeIndex_t m_endNode;
};
