#pragma once

class Node : public QGraphicsObject {
    Q_OBJECT

    Q_PROPERTY(qreal radius READ getRadius WRITE setRadius)

   public:
    enum { NodeType = UserType + 1 };

    enum InternalState { NONE, CURRENTLY_ANALYZED, ANALYZED };

    Node(size_t index);
    ~Node();

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
    QPainterPath shape() const override;
    int type() const override;

    void setFillColor(const QRgb c);
    void setOutlineColor(const QRgb c);
    void setSelectedOutlineColor(const QRgb c);

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

    InternalState getInternalState() const;

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

   public:
    static constexpr QRgb k_defaultFillColor{qRgb(255, 255, 255)};
    static constexpr QRgb k_defaultOutlineColor{qRgb(0, 0, 0)};
    static constexpr QRgb k_defaultSelectedOutlineColor{qRgb(41, 134, 243)};
    static constexpr QRgb k_defaultAnalyzedColor{qRgb(135, 245, 66)};
    static constexpr QRgb k_defaultCurrentlyAnalyzedColor{qRgb(255, 255, 61)};
    static constexpr QRgb k_defaultVisitedColor{qRgb(204, 204, 204)};
    static constexpr QRgb k_defaultUnvisitedOutlineColor{qRgb(240, 240, 240)};

   private:
    QRgb m_fill{qRgb(255, 255, 255)};
    QRgb m_outline{qRgb(0, 0, 0)};

    /*
    QRgb m_selectedOutline{qRgb(41, 134, 243)};
    */

    size_t m_index;
    double m_radius{1.};

    InternalState m_internalState{NONE};

   public:
    static constexpr double k_fullRadius{24.};
};
