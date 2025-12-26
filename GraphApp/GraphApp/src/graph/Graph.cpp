#include <pch.h>

#include "Graph.h"

#include "storage/AdjacencyList.h"
#include "storage/AdjacencyMatrix.h"

Graph::Graph(QWidget* parent) : QGraphicsView(parent), m_scene(new QGraphicsScene()) {
    setFrameStyle(QFrame::NoFrame);
    toggleDarkMode();

    QOpenGLWidget* glWidget = new QOpenGLWidget(this);
    glWidget->setFormat(QSurfaceFormat::defaultFormat());
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    setViewport(glWidget);

    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(m_scene);
    setSceneSize({30720, 17280});

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_scene->addItem(&m_graphManager);
    m_graphManager.setPos(0, 0);

    centerOn(m_scene->width() / 2, m_scene->height() / 2);

    m_zoomTextStopTimer.setSingleShot(true);
    connect(&m_zoomTextStopTimer, &QTimer::timeout, [this]() {
        m_shouldDrawZoom = false;
        m_zoomTextStopTimer.stop();
        viewport()->update();
    });

    m_edgeUpdateTimer.setSingleShot(true);
    connect(&m_edgeUpdateTimer, &QTimer::timeout, [this]() { m_graphManager.buildEdgeCache(); });
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

void Graph::toggleDarkMode() {
    constexpr auto black = qRgb(20, 20, 20);
    constexpr auto white = qRgb(255, 255, 255);

    if (m_darkMode) {
        setBackgroundBrush(QColor::fromRgb(white));
        m_graphManager.setNodeDefaultColor(white);
        m_graphManager.setNodeOutlineDefaultColor(black);

    } else {
        setBackgroundBrush(QColor::fromRgb(black));
        m_graphManager.setNodeDefaultColor(black);
        m_graphManager.setNodeOutlineDefaultColor(white);
    }

    m_darkMode = !m_darkMode;
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

    const auto storageType = m_graphManager.getGraphStorage()->type();
    if (storageType == IGraphStorage::Type::ADJACENCY_LIST) {
        invertedGraphManager.m_graphStorage = std::make_unique<AdjacencyList>();
    } else if (storageType == IGraphStorage::Type::ADJACENCY_MATRIX) {
        invertedGraphManager.m_graphStorage = std::make_unique<AdjacencyMatrix>();
    }

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
}

void Graph::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->modifiers() & Qt::AltModifier) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        m_isDragging = true;
    }

    QGraphicsView::mousePressEvent(event);
}

void Graph::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isDragging) {
        setDragMode(QGraphicsView::NoDrag);
        m_isDragging = false;

        m_edgeUpdateTimer.stop();
        m_edgeUpdateTimer.start(500);
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void Graph::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space) {
        emit spacePressed();
    } else if (event->key() == Qt::Key_Escape) {
        emit escapePressed();
    }

    QGraphicsView::keyPressEvent(event);
}

void Graph::drawForeground(QPainter* painter, const QRectF& rect) {
    painter->save();
    painter->resetTransform();

    drawZoomText(painter);
    drawWatermark(painter);

    painter->restore();

    QGraphicsView::drawForeground(painter, rect);
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
