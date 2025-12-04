#include <pch.h>

#include "Graph.h"

Graph::Graph(QWidget* parent) : QGraphicsView(parent) {
    setBackgroundBrush(Qt::white);

    QSurfaceFormat format;
    format.setSamples(4);
    format.setSwapInterval(1);

    QOpenGLWidget* glWidget = new QOpenGLWidget(this);
    glWidget->setFormat(format);

    setViewport(glWidget);
}

void Graph::onAdjacencyListChanged(const QString& text) {
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    resetAdjacencyList();
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
        const auto u = parts[0].toULongLong(&ok1);
        const auto v = parts[1].toULongLong(&ok2);

        const int cost = [&]() {
            if (parts.size() >= 3) {
                return parts[2].toInt();
            }

            return 0;
        }();

        if (ok1 && ok2) {
            if (u >= m_nodes.size()) {
                QMessageBox::warning(nullptr, "Bad value",
                                     QString("Node %1 doesn't exist!").arg(u));
                break;
            }

            if (v >= m_nodes.size()) {
                QMessageBox::warning(nullptr, "Bad value",
                                     QString("Node %1 doesn't exist!").arg(v));
                break;
            }

            auto& uNeighbours = m_adjacencyList[m_nodes[u]];
            if (uNeighbours.contains(m_nodes[v])) {
                QMessageBox::warning(nullptr, "Duplicate edge",
                                     QString("Edge from %1 to %2 already exists!").arg(u).arg(v));
                break;
            }

            uNeighbours.emplace(m_nodes[v]);
            addEdge(m_nodes[u], m_nodes[v], cost);
        }
    }
}

Node* Graph::getFirstSelectedNode() const {
    for (auto node : m_nodes) {
        if (node->isSelected()) {
            return node;
        }
    }

    return nullptr;
}

size_t Graph::getNodesCount() { return m_nodes.size(); }

const std::vector<Node*>& Graph::getNodes() const { return m_nodes; }

const std::unordered_map<Node*, std::unordered_set<Node*>>& Graph::getAdjacencyList() const {
    return m_adjacencyList;
}

int Graph::getZoomPercentage() { return static_cast<int>(m_currentZoomScale * 100); }

void Graph::resetGraph() {
    scene()->clear();

    m_adjacencyList.clear();
    m_edges.clear();
    m_nodes.clear();
}

void Graph::enableEditing() { m_editingEnabled = true; }

void Graph::disableEditing() { m_editingEnabled = false; }

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
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            setDragMode(QGraphicsView::RubberBandDrag);
            m_isSelecting = true;
        } else if (event->modifiers() & Qt::AltModifier) {
            setDragMode(QGraphicsView::ScrollHandDrag);
            m_isDragging = true;
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void Graph::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_isSelecting) {
            setDragMode(QGraphicsView::NoDrag);
            m_isSelecting = false;
        } else if (m_isDragging) {
            setDragMode(QGraphicsView::NoDrag);
            m_isDragging = false;
            emit movedGraph();
        } else {
            const auto scenePos = mapToScene(event->pos());
            addNode(scenePos);
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void Graph::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        removeSelectedNodes();
    } else if (event->key() == Qt::Key_Space) {
        emit spacePressed();
    }

    QGraphicsView::keyPressEvent(event);
}

void Graph::resizeEvent(QResizeEvent* event) {
    const auto sceneWidth = static_cast<int>(scene()->sceneRect().width());
    const auto sceneHeight = static_cast<int>(scene()->sceneRect().height());

    scene()->setSceneRect(0, 0, qMax(sceneWidth, width()), qMax(sceneHeight, height()));

    QGraphicsView::resizeEvent(event);
}

void Graph::addNode(const QPointF& pos) {
    if (!m_editingEnabled) {
        return;
    }

    const auto bounds = scene()->sceneRect();
    constexpr auto r = Node::k_radius;
    if (!bounds.adjusted(r, r, -r, -r).contains(pos)) {
        return;
    }

    for (auto node : m_nodes) {
        const auto delta = pos - node->pos();
        const auto distSq = delta.x() * delta.x() + delta.y() * delta.y();

        static constexpr auto minDist = 2. * Node::k_radius;
        if (distSq < (minDist * minDist)) {
            return;
        }
    }

    Node* node = new Node(m_nodes.size());
    node->setPos(pos);
    node->setParent(this);
    node->setAllNodesView(&m_nodes);

    connect(node, &Node::markedForErasure, this, &Graph::onNodeMarkedForErasure);

    scene()->addItem(node);
    m_nodes.push_back(node);

    m_edges.reserve(m_nodes.size() * m_nodes.size());
}

void Graph::removeNodeConnections(Node* node) {
    removeEdgesConnectedToNode(node);
    m_adjacencyList.erase(node);

    for (auto& [_, list] : m_adjacencyList) {
        list.erase(node);
    }
}

void Graph::removeSelectedNodes() {
    if (!m_editingEnabled) {
        return;
    }

    size_t index = 0;
    for (auto it = m_nodes.begin(); it != m_nodes.end();) {
        Node* node = *it;
        if (node->isSelected()) {
            removeNodeConnections(node);
            node->markForErasure();

            it = m_nodes.erase(it);
        } else {
            node->setIndex(index++);
            ++it;
        }
    }
}

void Graph::onNodeMarkedForErasure(Node* node) {
    scene()->removeItem(node);
    node->deleteLater();
}

void Graph::addEdge(Node* a, Node* b, int cost) {
    const auto edge = new Edge(a, b, cost);
    edge->setParent(this);

    connect(edge, &Edge::markedForErasure, this, &Graph::onEdgeMarkedForErasure);

    m_edges.push_back(edge);
    scene()->addItem(edge);
}

void Graph::removeEdgesConnectedToNode(Node* node) {
    for (auto it = m_edges.begin(); it != m_edges.end();) {
        Edge* edge = *it;
        if (edge->connectsNode(node)) {
            edge->markForErasure();

            it = m_edges.erase(it);
        } else {
            ++it;
        }
    }
}

void Graph::onEdgeMarkedForErasure(Edge* edge) {
    scene()->removeItem(edge);
    edge->deleteLater();
}

void Graph::resetAdjacencyList() {
    for (auto edge : m_edges) {
        edge->markForErasure();
    }

    m_edges.clear();
    m_adjacencyList.clear();
}
