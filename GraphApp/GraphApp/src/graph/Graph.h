#pragma once

#include "GraphManager.h"

class Graph : public QGraphicsView {
    Q_OBJECT

   public:
    Graph(QWidget* parent = nullptr);
    ~Graph();

    void buildFromAdjacencyListString(const QString& text);
    GraphManager& getGraphManager();
    void toggleDarkMode();

    void setSceneSize(QSize size);
    QSize getSceneSize() const;

    Graph* getInvertedGraph() const;

   signals:
    void spacePressed();
    void escapePressed();

   protected:
    void wheelEvent(QWheelEvent* event) final;

    void mousePressEvent(QMouseEvent* event) final;
    void mouseReleaseEvent(QMouseEvent* event) final;

    void keyPressEvent(QKeyEvent* event) final;

    void drawForeground(QPainter* painter, const QRectF& rect) final;

   private:
    int getZoomPercentage();

    QSize m_sceneSize{30720, 17280};
    QGraphicsScene* m_scene;
    GraphManager m_graphManager;

    bool m_isDragging{false};
    bool m_darkMode{false};
    bool m_shouldDrawZoom{false};

    QTimer m_zoomTextStopTimer;
    double m_currentZoomScale{1.};

    static constexpr double k_minScale = 0.1;
    static constexpr double k_maxScale = 5.;
    static constexpr double k_zoomStep = 0.1;
};
