#pragma once

#include "../node/Node.h"

class Edge : public QGraphicsObject {
    Q_OBJECT

    Q_PROPERTY(qreal progress READ getProgress WRITE setProgress)

   public:
    enum { EdgeType = UserType + 2 };

    Edge(Node* a, Node* b, int cost = 0);
    ~Edge();

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    int type() const override;

    bool connectsNode(Node* node) const;

    void setColor(const QColor& c);

    qreal getProgress() const;
    void setProgress(qreal p);

    bool isLoop() const;

   private:
    void connectNodeSignals(Node* node);

    void updatePosition();

    void markUnvisited();
    void markVisited(Node* parent);
    void markAnalyzed(Node* parent);

    void markAvailableInPathFindingPath(Node* node);
    void markPath(Node* parent);

    void unmark();

    void drawSelfLoopEdge(QPainter* painter);
    void drawEdge(QPainter* painter);

    Node* m_startNode;
    Node* m_endNode;
    int m_cost{0};

    QLineF m_line;

    QPolygonF m_arrowHead;
    static constexpr double k_arrowSize{15.};

    QColor m_color{Qt::black};
    QColor m_selectedColor{Qt::green};

    qreal m_progress = 0.;
};
