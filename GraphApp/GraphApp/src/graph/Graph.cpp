#include <pch.h>

#include "Graph.h"

Graph::Graph(QWidget* parent) : QGraphicsView(parent), m_scene(new QGraphicsScene()) {
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setFrameStyle(QFrame::NoFrame);
    toggleDarkMode();

    QOpenGLWidget* glWidget = new QOpenGLWidget(this);
    glWidget->setFormat(QSurfaceFormat::defaultFormat());
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    setViewport(glWidget);

    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(m_scene);
    setSceneSize({30720, 17280});
    centerOn(m_scene->width() / 2, m_scene->height() / 2);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_scene->addItem(&m_graphManager);
    m_graphManager.setPos(0, 0);
    m_graphManager.updateVisibleSceneRect();

    m_zoomTextStopTimer.setSingleShot(true);
    connect(&m_zoomTextStopTimer, &QTimer::timeout, [this]() {
        m_shouldDrawZoom = false;
        m_zoomTextStopTimer.stop();
        viewport()->update();
    });

    m_edgeUpdateTimer.setSingleShot(true);
    connect(&m_edgeUpdateTimer, &QTimer::timeout, [this]() { m_graphManager.buildEdgeCache(); });

    m_leftArrowStopTimer.setSingleShot(true);
    connect(&m_leftArrowStopTimer, &QTimer::timeout, [this]() {
        m_shouldDrawLeftArrow = false;
        m_leftArrowStopTimer.stop();
        viewport()->update();
    });

    m_rightArrowStopTimer.setSingleShot(true);
    connect(&m_rightArrowStopTimer, &QTimer::timeout, [this]() {
        m_shouldDrawRightArrow = false;
        m_rightArrowStopTimer.stop();
        viewport()->update();
    });

    m_resumedAlgorithmStopTimer.setSingleShot(true);
    connect(&m_resumedAlgorithmStopTimer, &QTimer::timeout, [this]() {
        m_shouldDrawResumedAlgorithm = false;
        m_resumedAlgorithmStopTimer.stop();
        viewport()->update();
    });

    m_pausedAlgorithmStopTimer.setSingleShot(true);
    connect(&m_pausedAlgorithmStopTimer, &QTimer::timeout, [this]() {
        m_shouldDrawPausedAlgorithm = false;
        m_pausedAlgorithmStopTimer.stop();
        viewport()->update();
    });

    setupShortcuts();
}

Graph::~Graph() {
    m_graphManager.m_edgeFuture.cancel();
    m_graphManager.m_edgeFuture.waitForFinished();

    m_scene->deleteLater();
}

bool Graph::buildFromAdjacencyListString(const QString& text) {
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    m_graphManager.resetAdjacencyMatrix();
    m_graphManager.resizeAdjacencyMatrix(m_graphManager.getNodesCount());

    m_graphManager.evaluateStorageStrategy(lines.size());

    for (QString& line : lines) {
        line = line.trimmed();
        if (line.isEmpty()) {
            continue;
        }

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            continue;
        }

        bool ok1, ok2;
        const auto u = parts[0].toUInt(&ok1);
        const auto v = parts[1].toUInt(&ok2);

        const auto cost = [&]() {
            if (parts.size() >= 3) {
                return parts[2].toInt();
            }

            return 0;
        }();

        if (ok1 && ok2) {
            if (u >= m_graphManager.getNodesCount()) {
                QMessageBox::warning(nullptr, "Bad value",
                                     QString("Node %1 doesn't exist!").arg(u));
                return false;
            }

            if (v >= m_graphManager.getNodesCount()) {
                QMessageBox::warning(nullptr, "Bad value",
                                     QString("Node %1 doesn't exist!").arg(v));
                return false;
            }

            if (m_graphManager.hasNeighbour(u, v)) {
                QMessageBox::warning(nullptr, "Duplicate edge",
                                     QString("Edge from %1 to %2 already exists!").arg(u).arg(v));
                return false;
            }

            if (!m_graphManager.getAllowLoops() && u == v) {
                QMessageBox::warning(nullptr, "Loop not allowed",
                                     QString("Loops are not allowed (Node %1)!").arg(u));
                return false;
            }

            if (!m_graphManager.getOrientedGraph()) {
                if (m_graphManager.hasNeighbour(v, u)) {
                    QMessageBox::warning(nullptr, "Duplicate edge",
                                         QString("Edge from %1 to %2 already exists!\nBecause this "
                                                 "is an unoriented graph")
                                             .arg(v)
                                             .arg(u));
                    return false;
                }
            }

            m_graphManager.addEdge(u, v, cost);
            if (!m_graphManager.getOrientedGraph()) {
                m_graphManager.addEdge(v, u, cost);
            }
        }
    }

    return true;
}

GraphManager& Graph::getGraphManager() { return m_graphManager; }

QRgb Graph::getDefaultNodeColor() const { return m_darkMode ? k_black : k_white; }

QRgb Graph::getDefaultNodeOutlineColor() const { return m_darkMode ? k_white : k_black; }

void Graph::toggleDarkMode() {
    m_darkMode = !m_darkMode;

    setBackgroundBrush(QColor::fromRgb(getDefaultNodeColor()));
    m_graphManager.setNodeDefaultColor(getDefaultNodeColor());
    m_graphManager.setNodeOutlineDefaultColor(getDefaultNodeOutlineColor());
}

void Graph::setSceneSize(QSize size) {
    m_graphManager.reset();
    m_graphManager.setSceneDimensions(size);

    m_scene->setSceneRect(0, 0, size.width(), size.height());
}

QSize Graph::getSceneSize() const { return m_scene->sceneRect().size().toSize(); }

Graph* Graph::getInvertedGraph() const {
    if (m_graphManager.getNodesCount() == 0) {
        QMessageBox::warning(nullptr, "Error", "Cannot invert a graph without nodes.",
                             QMessageBox::Ok);
        return nullptr;
    }

    if (!m_graphManager.getOrientedGraph()) {
        QMessageBox::warning(nullptr, "Warning", "Cannot invert an unoriented graph.",
                             QMessageBox::Ok);
        return nullptr;
    }

    Graph* invertedGraph = new Graph();
    if (!m_darkMode) {
        invertedGraph->toggleDarkMode();
    }

    invertedGraph->m_scene->setSceneRect(m_scene->sceneRect());

    auto& invertedGraphManager = invertedGraph->m_graphManager;
    invertedGraphManager.setSceneDimensions(m_scene->sceneRect().size().toSize());
    invertedGraphManager.setGraphStorageType(m_graphManager.getGraphStorage()->type());

    invertedGraphManager.setAllowEditing(m_graphManager.getAllowEditing());
    invertedGraphManager.setAllowLoops(m_graphManager.getAllowLoops());
    invertedGraphManager.setDrawNodesEnabled(m_graphManager.getDrawNodesEnabled());
    invertedGraphManager.setDrawEdgesEnabled(m_graphManager.getDrawEdgesEnabled());

    invertedGraphManager.setCollisionsCheckEnabled(false);
    for (const auto& node : m_graphManager.m_nodes) {
        invertedGraphManager.addNode(node.getPosition());
    }
    invertedGraphManager.setCollisionsCheckEnabled(true);

    invertedGraphManager.resizeAdjacencyMatrix(m_graphManager.m_nodes.size());
    for (NodeIndex_t nodeIndex = 0; nodeIndex < m_graphManager.m_nodes.size(); ++nodeIndex) {
        m_graphManager.m_graphStorage->forEachOutgoingEdgeWithOpposites(
            nodeIndex, [&](NodeIndex_t neighbourIndex, CostType_t cost) {
                invertedGraphManager.addEdge(neighbourIndex, nodeIndex, cost);
            });
    }

    return invertedGraph;
}

void Graph::notifyLeftArrowPressed() {
    m_shouldDrawLeftArrow = true;
    m_shouldDrawRightArrow = false;

    m_rightArrowStopTimer.stop();
    m_leftArrowStopTimer.start(500);
}

void Graph::notifyRightArrowPressed() {
    m_shouldDrawRightArrow = true;
    m_shouldDrawLeftArrow = false;

    m_leftArrowStopTimer.stop();
    m_rightArrowStopTimer.start(500);
}

void Graph::notifyAlgorithmResumed() {
    m_shouldDrawResumedAlgorithm = true;
    m_shouldDrawPausedAlgorithm = false;

    m_pausedAlgorithmStopTimer.stop();
    m_resumedAlgorithmStopTimer.start(1200);

    viewport()->update();
}

void Graph::notifyAlgorithmPaused() {
    m_shouldDrawPausedAlgorithm = true;
    m_shouldDrawResumedAlgorithm = false;

    m_resumedAlgorithmStopTimer.stop();
    m_pausedAlgorithmStopTimer.start(1200);

    viewport()->update();
}

void Graph::wheelEvent(QWheelEvent* event) {
    double newScale = m_currentZoomScale;

    if (event->angleDelta().y() > 0) {
        newScale += k_zoomStep;
    } else {
        newScale -= k_zoomStep;
    }

    newScale = qBound(k_minScale, newScale, k_maxScale);

    const auto factor = newScale / m_currentZoomScale;

    scale(factor, factor);
    m_currentZoomScale = newScale;

    m_shouldDrawZoom = true;
    m_zoomTextStopTimer.stop();
    m_zoomTextStopTimer.start(1500);

    m_edgeUpdateTimer.stop();
    m_edgeUpdateTimer.start(500);

    m_graphManager.updateVisibleSceneRect();
}

void Graph::scrollContentsBy(int dx, int dy) {
    QGraphicsView::scrollContentsBy(dx, dy);
    m_graphManager.updateVisibleSceneRect();
}

void Graph::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);

    m_graphManager.updateVisibleSceneRect();

    m_edgeUpdateTimer.stop();
    m_edgeUpdateTimer.start(500);
}

void Graph::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        m_isDragging = true;

        event = new QMouseEvent(QEvent::MouseButtonPress, event->pos(), event->globalPosition(),
                                Qt::LeftButton, Qt::LeftButton, Qt::AltModifier);
    } else if (event->button() == Qt::LeftButton && event->modifiers() & Qt::AltModifier) {
        if (m_isDragging) {
            return;
        } else {
            setDragMode(QGraphicsView::ScrollHandDrag);
            m_isDragging = true;
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void Graph::mouseReleaseEvent(QMouseEvent* event) {
    if ((event->button() == Qt::RightButton ||
         (event->button() == Qt::LeftButton && event->modifiers() & Qt::AltModifier)) &&
        m_isDragging) {
        setDragMode(QGraphicsView::NoDrag);
        m_isDragging = false;

        m_edgeUpdateTimer.stop();
        m_edgeUpdateTimer.start(500);
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void Graph::drawForeground(QPainter* painter, const QRectF& rect) {
    painter->save();
    painter->resetTransform();

    drawZoomText(painter);
    drawWatermark(painter);
    drawLeftArrow(painter);
    drawRightArrow(painter);
    drawResumedAlgorithm(painter);
    drawPausedAlgorithm(painter);

    painter->restore();

    QGraphicsView::drawForeground(painter, rect);
}

void Graph::setupShortcuts() {
    new QShortcut(Qt::Key_Left, this, [this]() { emit leftArrowPressed(); });
    new QShortcut(Qt::Key_Right, this, [this]() { emit rightArrowPressed(); });
    new QShortcut(Qt::Key_Space, this, [this]() { emit spacePressed(); });
    new QShortcut(Qt::Key_Return, this, [this]() { emit enterPressed(); });

    new QShortcut(Qt::Key_Delete, this, [this]() {
        if (m_graphManager.m_editingEnabled) {
            m_graphManager.removeSelectedNodes();
        }
    });

    new QShortcut(Qt::Key_Escape, this, [this]() {
        if (m_graphManager.runningAlgorithm()) {
            m_graphManager.cancelAlgorithms();
        } else {
            m_graphManager.deselectNodes();
        }
    });

    new QShortcut(Qt::Key_P, this, [this]() {
        if (m_graphManager.runningAlgorithm() && m_graphManager.m_algorithmInfoTextSize < 40) {
            m_graphManager.m_algorithmInfoTextItem->setFont(
                QFont(QApplication::font().family(), ++m_graphManager.m_algorithmInfoTextSize));
        }
    });

    new QShortcut(Qt::Key_L, this, [this]() {
        if (m_graphManager.runningAlgorithm() && m_graphManager.m_algorithmInfoTextSize > 6) {
            m_graphManager.m_algorithmInfoTextItem->setFont(
                QFont(QApplication::font().family(), --m_graphManager.m_algorithmInfoTextSize));
        }
    });
}

int Graph::getZoomPercentage() { return static_cast<int>(std::round(m_currentZoomScale * 100)); }

void Graph::drawZoomText(QPainter* painter) {
    if (!m_shouldDrawZoom) {
        return;
    }

    const QString zoomText = QString("Zoom: %1%").arg(getZoomPercentage());

    QFont font = painter->font();
    font.setPixelSize(20);
    painter->setFont(font);

    QFontMetrics fm(font);
    constexpr int padding = 5;

    const QSize textSize(fm.horizontalAdvance(zoomText), fm.height());
    const auto viewRect = viewport()->rect();
    const QRect textRect(viewRect.right() - textSize.width() - padding * 2 - 10,
                         viewRect.top() + 10, textSize.width() + padding * 2,
                         textSize.height() + padding * 2);

    painter->setPen(Qt::black);
    painter->setBrush(QColor(255, 255, 255, 200));

    painter->drawRect(textRect);
    painter->drawText(textRect.adjusted(padding, padding, -padding, -padding), Qt::AlignCenter,
                      zoomText);
}

void Graph::drawWatermark(QPainter* painter) {
    static const auto watermarkText = QStringLiteral("github.com/mariusunitbv/graphapp");

    QFont font = painter->font();
    font.setPixelSize(10);
    painter->setFont(font);

    QFontMetrics fm(font);

    const QSize textSize(fm.horizontalAdvance(watermarkText), fm.height());
    const auto viewRect = viewport()->rect();
    const QRect textRect(viewRect.left() + 10, viewRect.bottom() - textSize.height() - 10,
                         textSize.width(), textSize.height());

    painter->setPen(m_darkMode ? Qt::white : Qt::black);
    painter->drawText(textRect, Qt::AlignCenter, watermarkText);
}

void Graph::drawRightArrow(QPainter* painter) {
    if (!m_shouldDrawRightArrow) {
        return;
    }

    const auto viewRect = viewport()->rect();

    const int triangleHeight = 50;
    const int triangleWidth = 40;

    int paddingLeft = 0;
    if (m_shouldDrawPausedAlgorithm || m_shouldDrawResumedAlgorithm) {
        paddingLeft = 50;
    }

    const int x = viewRect.left() + 10 + paddingLeft;
    const int y = viewRect.bottom() - triangleHeight - 30;

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_darkMode ? Qt::white : Qt::black);

    QPoint first[3] = {QPoint(x, y), QPoint(x, y + triangleHeight),
                       QPoint(x + triangleWidth, y + triangleHeight / 2)};

    QPoint second[3] = {QPoint(x + triangleWidth, y), QPoint(x + triangleWidth, y + triangleHeight),
                        QPoint(x + 2 * triangleWidth, y + triangleHeight / 2)};

    painter->drawPolygon(first, 3);
    painter->drawPolygon(second, 3);
}

void Graph::drawLeftArrow(QPainter* painter) {
    if (!m_shouldDrawLeftArrow) {
        return;
    }

    const auto viewRect = viewport()->rect();

    const int triangleHeight = 50;
    const int triangleWidth = 40;

    int paddingLeft = 0;
    if (m_shouldDrawPausedAlgorithm || m_shouldDrawResumedAlgorithm) {
        paddingLeft = 50;
    }

    const int x = viewRect.left() + 10 + paddingLeft;
    const int y = viewRect.bottom() - triangleHeight - 30;

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_darkMode ? Qt::white : Qt::black);

    QPoint first[3] = {QPoint(x + triangleWidth, y), QPoint(x + triangleWidth, y + triangleHeight),
                       QPoint(x, y + triangleHeight / 2)};

    QPoint second[3] = {QPoint(x + 2 * triangleWidth, y),
                        QPoint(x + 2 * triangleWidth, y + triangleHeight),
                        QPoint(x + triangleWidth, y + triangleHeight / 2)};

    painter->drawPolygon(first, 3);
    painter->drawPolygon(second, 3);
}

void Graph::drawResumedAlgorithm(QPainter* painter) {
    if (!m_shouldDrawResumedAlgorithm) {
        return;
    }

    const auto viewRect = viewport()->rect();

    const int triangleHeight = 50;
    const int triangleWidth = 40;

    const int x = viewRect.left() + 10;
    const int y = viewRect.bottom() - triangleHeight - 30;

    QPoint points[3] = {QPoint(x, y), QPoint(x, y + triangleHeight),
                        QPoint(x + triangleWidth, y + triangleHeight / 2)};

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_darkMode ? Qt::white : Qt::black);
    painter->drawPolygon(points, 3);
}

void Graph::drawPausedAlgorithm(QPainter* painter) {
    if (!m_shouldDrawPausedAlgorithm) {
        return;
    }

    const auto viewRect = viewport()->rect();

    const int barWidth = 15;
    const int barHeight = 50;
    const int spacing = 10;

    const int x = viewRect.left() + 10;
    const int y = viewRect.bottom() - barHeight - 30;

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_darkMode ? Qt::white : Qt::black);

    painter->drawRect(x, y, barWidth, barHeight);
    painter->drawRect(x + barWidth + spacing, y, barWidth, barHeight);
}
