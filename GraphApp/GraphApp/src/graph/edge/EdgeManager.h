#pragma once

class EdgeManager : public QGraphicsObject {
    Q_OBJECT

   public:
    EdgeManager();

    void setSceneDimensions(qreal width, qreal height);

   protected:
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) override;

   private:
    bool isVisibleInScene(const QRect& rect) const;

    QRect m_boundingRect{};
    QRect m_sceneRect{};
    QTimer m_sceneUpdateTimer;
};
