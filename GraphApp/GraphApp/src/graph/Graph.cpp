#include <pch.h>

#include "Graph.h"

Graph::Graph(QWidget* parent) : QGraphicsView(parent), m_scene(new QGraphicsScene()) {
    setFrameStyle(QFrame::NoFrame);
    toggleDarkMode();

    QOpenGLWidget* glWidget = new QOpenGLWidget(this);
    glWidget->setFormat(QSurfaceFormat::defaultFormat());

    setViewport(glWidget);

    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(m_scene);
    setSceneSize(m_sceneSize);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_scene->addItem(&m_graphManager);
    m_graphManager.setPos(0, 0);

    centerOn(m_scene->width() / 2, m_scene->height() / 2);

    connect(&m_zoomTextStopTimer, &QTimer::timeout, [this]() {
        m_shouldDrawZoom = false;
        m_zoomTextStopTimer.stop();
        viewport()->update();
    });
}

Graph::~Graph() { m_scene->deleteLater(); }

void Graph::buildFromAdjacencyListString(const QString& text) {
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    m_graphManager.resetAdjacencyMatrix();
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
                break;
            }

            if (v >= m_graphManager.getNodesCount()) {
                QMessageBox::warning(nullptr, "Bad value",
                                     QString("Node %1 doesn't exist!").arg(v));
                break;
            }

            if (m_graphManager.hasNeighbour(u, v)) {
                QMessageBox::warning(nullptr, "Duplicate edge",
                                     QString("Edge from %1 to %2 already exists!").arg(u).arg(v));
                break;
            }

            if (!m_graphManager.getAllowLoops() && u == v) {
                QMessageBox::warning(nullptr, "Loop not allowed",
                                     QString("Loops are not allowed (Node %1)!").arg(u));
                break;
            }

            if (!m_graphManager.getOrientedGraph()) {
                if (m_graphManager.hasNeighbour(v, u)) {
                    QMessageBox::warning(nullptr, "Duplicate edge",
                                         QString("Edge from %1 to %2 already exists!\nBecause this"
                                                 "is an unoriented graph")
                                             .arg(v)
                                             .arg(u));
                    break;
                }
            }

            m_graphManager.addEdge(u, v, cost);
        }
    }
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
    m_graphManager.setSceneDimensions(size.width(), size.height());

    m_scene->setSceneRect(0, 0, size.width(), size.height());
    m_sceneSize = size;
}

QSize Graph::getSceneSize() const { return m_sceneSize; }

Graph* Graph::getInvertedGraph() const {
    if (!m_graphManager.getOrientedGraph()) {
        QMessageBox::warning(nullptr, "Warning", "Cannot invert an unoriented graph",
                             QMessageBox::Ok);
        return nullptr;
    }

    return nullptr;
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
    if (m_shouldDrawZoom) {
        painter->save();
        painter->resetTransform();

        const QString zoomText = QString("Zoom: %1%").arg(getZoomPercentage());

        QFont font = painter->font();
        font.setPixelSize(20);
        painter->setFont(font);

        QFontMetrics fm(font);
        const int padding = 5;

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

        painter->restore();
    }

    QGraphicsView::drawForeground(painter, rect);
}

int Graph::getZoomPercentage() { return static_cast<int>(std::round(m_currentZoomScale * 100)); }
