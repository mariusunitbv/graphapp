#include <pch.h>

#include "Graph.h"

Graph::Graph(QWidget* parent) : QGraphicsView(parent), m_scene(new QGraphicsScene()) {
    setBackgroundBrush(Qt::white);

    QOpenGLWidget* glWidget = new QOpenGLWidget(this);
    glWidget->setFormat(QSurfaceFormat::defaultFormat());

    setViewport(glWidget);

    m_scene->setSceneRect(0, 0, 50000, 50000);
    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(m_scene);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_scene->addItem(&m_graphManager);
    m_graphManager.setSceneDimensions(m_scene->width(), m_scene->height());
    m_graphManager.setPos(0, 0);

    centerOn(m_scene->width() / 2, m_scene->height() / 2);
}

Graph::~Graph() { m_scene->deleteLater(); }

void Graph::onAdjacencyListChanged(const QString& text) {
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

int Graph::getZoomPercentage() { return static_cast<int>(m_currentZoomScale * 100); }

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

    double factor = newScale / m_currentZoomScale;

    scale(factor, factor);
    m_currentZoomScale = newScale;

    emit zoomChanged();
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
