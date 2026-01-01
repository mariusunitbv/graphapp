#pragma once

#include "GraphManager.h"

class Graph : public QGraphicsView {
    Q_OBJECT

   public:
    Graph(QWidget* parent = nullptr);
    ~Graph();

    bool buildFromAdjacencyListString(const QString& text);
    GraphManager& getGraphManager();

    QRgb getDefaultNodeColor() const;
    QRgb getDefaultNodeOutlineColor() const;
    void toggleDarkMode();

    void setSceneSize(QSize size);
    QSize getSceneSize() const;

    Graph* getInvertedGraph() const;

   signals:
    void spacePressed();
    void escapePressed();

   protected:
    void wheelEvent(QWheelEvent* event) final;
    void scrollContentsBy(int dx, int dy) final;
    void resizeEvent(QResizeEvent* event) final;

    void mousePressEvent(QMouseEvent* event) final;
    void mouseReleaseEvent(QMouseEvent* event) final;

    void keyReleaseEvent(QKeyEvent* event) final;

    void drawForeground(QPainter* painter, const QRectF& rect) final;

   private:
    int getZoomPercentage();

    void drawZoomText(QPainter* painter);
    void drawWatermark(QPainter* painter);

    QGraphicsScene* m_scene;
    GraphManager m_graphManager;
    QTimer m_edgeUpdateTimer;

    bool m_isDragging{false};
    bool m_darkMode{false};
    bool m_shouldDrawZoom{false};

    QTimer m_zoomTextStopTimer;
    double m_currentZoomScale{1.};

    static constexpr double k_minScale = 0.1;
    static constexpr double k_maxScale = 5.;
    static constexpr double k_zoomStep = 0.1;

    static constexpr QRgb k_white = qRgb(255, 255, 255);
    static constexpr QRgb k_black = qRgb(20, 20, 20);
};
