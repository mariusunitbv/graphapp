#pragma once

class Node : public QGraphicsObject {
    Q_OBJECT

   public:
    enum class State : uint8_t {
        NONE,
        UNVISITED,
        VISITED,
        CURRENTLY_ANALYZING,
        ANALYZED,
        PATH,
        CONNECTED_COMPONENT
    };

    Node(size_t index);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
    QPainterPath shape() const override;

    void setFillColor(const QRgb c);
    QRgb getFillColor() const;

    void setOutlineColor(const QRgb c);
    void setOutlineWidth(float width);
    void setOpacity(qreal opacity);
    void setScale(qreal scale);

    void setLabel(const QString& label);
    QString getLabel() const;

    void setAllNodesView(const std::vector<Node*>* allNodesView);

    void markUnvisited();
    void markVisited();
    void markVisitedButNotAnalyzedAnymore();
    void markCurrentlyAnalyzed();
    void markAnalyzed();
    void markPartOfConnectedComponent(QRgb c);

    void markAvailableInPathFinding();
    void markUnreachable();
    void markPath();

    void markForErasure();
    void unmark();

    void setIndex(size_t index);
    size_t getIndex() const;

    State getState() const;

    void setAnimationDisabled(bool disabled);
    bool isAnimationDisabled() const;

   signals:
    void changedPosition();
    void selectionChanged(bool selected);

    void markedUnvisited();
    void markedVisited();
    void markedAnalyzed();
    void markedPartOfConnectedComponent(QRgb c);

    void markedAvailableInPathFinding(Node* node);
    void markedPath();

    void markedForErasure(Node* node);
    void unmarked();

   protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent*) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;

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

    float m_outlineWidth{1.5f};

    size_t m_index{std::numeric_limits<size_t>::max()};
    State m_state{State::NONE};
    bool m_animationDisabled{false};

    const std::vector<Node*>* m_allNodesView{nullptr};
    QString m_label;

    QPointer<QVariantAnimation> m_colorAnimation;
    QPointer<QVariantAnimation> m_outlineColorAnimation;
    QPointer<QVariantAnimation> m_opacityAnimation;

   public:
    static constexpr double k_radius{24.};
};
