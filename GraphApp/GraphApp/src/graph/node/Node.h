#pragma once

class Node : public QGraphicsObject {
    Q_OBJECT

    Q_PROPERTY(qreal radius READ getRadius WRITE setRadius)

   public:
    enum { NodeType = UserType + 1 };

    Node(size_t index);
    ~Node();

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
    QPainterPath shape() const override;
    int type() const override;

    void setFillColor(const QColor& c);
    void setOutlineColor(const QColor& c);
    void setSelectedOutlineColor(const QColor& c);

    void markUnvisited();
    void markVisited(Node* parent);
    void markVisitedButNotAnalyzedAnymore();
    void markCurrentlyAnalyzed();
    void markAnalyzed(Node* parent);

    void markAvailableInPathFinding();
    void markPath(Node* parent);

    void unmark();

    void setIndex(size_t index);
    size_t getIndex() const;

    void setRadius(double radius);
    double getRadius() const;

   signals:
    void positionChanged();

    void markedUnvisited();
    void markedVisited(Node* parent);
    void markedAnalyzed(Node* parent);

    void markedAvailableInPathFinding(Node* node);
    void markedPath(Node* parent);

    void unmarked();

   protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

   private:
    QColor m_fill{Qt::white};
    QColor m_outline{Qt::black};
    QColor m_selectedOutline{QColor("#2986F3")};

    size_t m_index;
    double m_radius{1.};

   public:
    static constexpr double k_fullRadius{24.};
};
