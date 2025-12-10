#pragma once

#include "GraphManager.h"

class Graph : public QGraphicsView {
    Q_OBJECT

   public:
    Graph(QWidget* parent = nullptr);
    ~Graph();

    void onAdjacencyListChanged(const QString& text);
    GraphManager& getGraphManager();
    int getZoomPercentage();

    Graph* getCopy() const;
    Graph* getInvertedGraph() const;

   signals:
    void zoomChanged();
    void spacePressed();
    void escapePressed();

   protected:
    void wheelEvent(QWheelEvent* event) final;

    void mousePressEvent(QMouseEvent* event) final;
    void mouseReleaseEvent(QMouseEvent* event) final;

    void keyPressEvent(QKeyEvent* event) final;

   private:
    QGraphicsScene* m_scene;
    GraphManager m_graphManager;

    bool m_isDragging{false};
    double m_currentZoomScale{1.};

    static constexpr double k_minScale = 0.25;
    static constexpr double k_maxScale = 5.;
    static constexpr double k_zoomStep = 0.25;
};
