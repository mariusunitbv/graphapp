#pragma once

class Node : public QGraphicsObject {
    Q_OBJECT

   public:
    enum class State : uint8_t { NONE, UNVISITED, VISITED, CURRENTLY_ANALYZED, ANALYZED, PATH };

    Node(size_t index);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
    QPainterPath shape() const override;

    void setFillColor(const QRgb c);
    void setOutlineColor(const QRgb c, bool ignoreSelection = false);
    void setOutlineWidth(float width);
    void setOpacity(qreal opacity);
    void setScale(qreal scale);

    void setAllNodesView(const std::vector<Node*>* allNodesView);

    void markUnvisited();
    void markVisited(Node* parent = nullptr);
    void markVisitedButNotAnalyzedAnymore();
    void markCurrentlyAnalyzed();
    void markAnalyzed();

    void markAvailableInPathFinding();
    void markUnreachable();
    void markPath(Node* parent);

    void markForErasure();
    void unmark();

    void setIndex(size_t index);
    size_t getIndex() const;

    State getState() const;

   signals:
    void changedPosition();
    void selectionChanged(bool selected);

    void markedUnvisited();
    void markedVisited(Node* parent);
    void markedAnalyzed(Node* parent);

    void markedAvailableInPathFinding(Node* node);
    void markedPath(Node* parent);

    void markedForErasure(Node* node);
    void unmarked();

   protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

   public:
    static constexpr QRgb k_defaultFillColor{qRgb(255, 255, 255)};
    static constexpr QRgb k_defaultOutlineColor{qRgb(0, 0, 0)};
    static constexpr QRgb k_defaultSelectedOutlineColor{qRgb(41, 134, 243)};
    static constexpr QRgb k_defaultAnalyzedColor{qRgb(135, 245, 66)};
    static constexpr QRgb k_defaultCurrentlyAnalyzedColor{qRgb(255, 255, 61)};
    static constexpr QRgb k_defaultVisitedColor{qRgb(204, 204, 204)};
    static constexpr QRgb k_defaultUnvisitedOutlineColor{qRgb(240, 240, 240)};
    static constexpr QRgb k_defaultUnreachableColor{qRgb(255, 69, 69)};

   private:
    QPointF getGoodPositionWhenMoving(const QPointF& desiredPos);

    QRgb m_fill{qRgb(255, 255, 255)};
    QRgb m_outline{qRgb(0, 0, 0)};
    QRgb m_outlineBackup{qRgb(0, 0, 0)};

    float m_outlineWidth{1.5f};

    size_t m_index{std::numeric_limits<size_t>::max()};
    State m_state{State::NONE};

    const std::vector<Node*>* m_allNodesView{nullptr};

   public:
    static constexpr double k_radius{24.};
};
