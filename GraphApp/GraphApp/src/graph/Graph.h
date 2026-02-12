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

    void notifyLeftArrowPressed();
    void notifyRightArrowPressed();
    void notifyAlgorithmResumed();
    void notifyAlgorithmPaused();

   signals:
    void leftArrowPressed();
    void rightArrowPressed();
    void spacePressed();
    void enterPressed();

   protected:
    void wheelEvent(QWheelEvent* event) final;
    void scrollContentsBy(int dx, int dy) final;
    void resizeEvent(QResizeEvent* event) final;

    void mousePressEvent(QMouseEvent* event) final;
    void mouseReleaseEvent(QMouseEvent* event) final;

    void drawForeground(QPainter* painter, const QRectF& rect) final;

   private:
    void setupShortcuts();

    int getZoomPercentage();

    void drawZoomText(QPainter* painter);
    void drawWatermark(QPainter* painter);
    void drawLeftArrow(QPainter* painter);
    void drawRightArrow(QPainter* painter);
    void drawResumedAlgorithm(QPainter* painter);
    void drawPausedAlgorithm(QPainter* painter);

    QGraphicsScene* m_scene;
    GraphManager m_graphManager;
    QTimer m_edgeUpdateTimer;

    bool m_isDragging{false};
    bool m_darkMode{false};
    bool m_shouldDrawZoom{false};
    bool m_shouldDrawLeftArrow{false};
    bool m_shouldDrawRightArrow{false};
    bool m_shouldDrawResumedAlgorithm{false};
    bool m_shouldDrawPausedAlgorithm{false};

    QTimer m_zoomTextStopTimer;
    double m_currentZoomScale{1.};

    QTimer m_leftArrowStopTimer;
    QTimer m_rightArrowStopTimer;
    QTimer m_resumedAlgorithmStopTimer;
    QTimer m_pausedAlgorithmStopTimer;

    static constexpr double k_minScale = 0.1;
    static constexpr double k_maxScale = 5.;
    static constexpr double k_zoomStep = 0.1;

    static constexpr QRgb k_white = qRgb(255, 255, 255);
    static constexpr QRgb k_black = qRgb(20, 20, 20);
};
