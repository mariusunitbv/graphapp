#include <pch.h>

#include "Graph.h"

#include "../random/Random.h"

Graph::Graph(QWidget* parent) : QGraphicsView(parent), m_scene(new QGraphicsScene()) {
    setBackgroundBrush(Qt::white);

    QOpenGLWidget* glWidget = new QOpenGLWidget(this);
    glWidget->setFormat(QSurfaceFormat::defaultFormat());

    setViewport(glWidget);

    m_scene->setSceneRect(0, 0, 100000, 100000);
    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(m_scene);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_scene->addItem(&m_nodeManager);
    m_nodeManager.setSceneDimensions(m_scene->width(), m_scene->height());
    m_nodeManager.setPos(0, 0);
}

Graph::~Graph() { m_scene->deleteLater(); }

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
            /* if (u >= m_nodes.size()) {
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

             if (!m_allowLoops && u == v) {
                 QMessageBox::warning(nullptr, "Loop not allowed",
                                      QString("Loops are not allowed (Node %1)!").arg(u));
                 break;
             }

             if (!m_orientedGraph) {
                 if (m_adjacencyList[m_nodes[v]].contains(m_nodes[u])) {
                     QMessageBox::warning(nullptr, "Duplicate edge",
                                          QString("Edge from %1 to %2 already exists!\nBecause this
             " "is an unoriented graph") .arg(v) .arg(u)); break;
                 }

                 m_adjacencyList[m_nodes[v]].emplace(m_nodes[u]);
             }*/

            // uNeighbours.emplace(m_nodes[v]);
            m_nodeManager.addEdge(u, v, cost);
        }
    }
}

NodeManager& Graph::getNodeManager() { return m_nodeManager; }

Node* Graph::getFirstSelectedNode() const {
    for (auto node : m_nodes) {
        if (node->isSelected()) {
            return node;
        }
    }

    return nullptr;
}

size_t Graph::getNodesCount() const { return m_nodeManager.getNodesCount(); }

const std::vector<Node*>& Graph::getNodes() const { return m_nodes; }

Node* Graph::getRandomNode() const {
    if (getNodesCount() == 0) {
        return 0;
    }

    return m_nodes[Random::get().getSize(0, getNodesCount() - 1)];
}

QPointF Graph::getGraphSize() const { return {m_scene->width(), m_scene->height()}; }

const std::vector<Edge*>& Graph::getEdges() const { return m_edges; }

const std::unordered_map<Node*, std::unordered_set<Node*>>& Graph::getAdjacencyList() const {
    return m_adjacencyList;
}

int Graph::getZoomPercentage() { return static_cast<int>(m_currentZoomScale * 100); }

void Graph::resetGraph() { removeAllNodes(); }

void Graph::enableEditing() { m_editingEnabled = true; }

void Graph::disableEditing() { m_editingEnabled = false; }

void Graph::setAnimationsDisabled(bool disabled) {
    m_animationsDisabled = disabled;
    for (auto node : m_nodes) {
        node->setAnimationDisabled(disabled);
    }
}

bool Graph::getAnimationsDisabled() const { return m_animationsDisabled; }

void Graph::setAllowLoops(bool allow) { m_allowLoops = allow; }

bool Graph::getAllowLoops() const { return m_allowLoops; }

void Graph::setOrientedGraph(bool oriented) { m_orientedGraph = oriented; }

bool Graph::getOrientedGraph() const { return m_orientedGraph; }

Graph* Graph::getCopy() const {
    auto copiedGraph = new Graph();

    copiedGraph->setAnimationsDisabled(m_animationsDisabled);
    copiedGraph->setAllowLoops(m_allowLoops);
    copiedGraph->setOrientedGraph(m_orientedGraph);

    for (auto node : m_nodes) {
        copiedGraph->addNode(node->pos());
    }

    copiedGraph->reserveEdges(m_edges.size());
    for (auto edge : m_edges) {
        Node* u = copiedGraph->m_nodes[edge->getStartNode()->getIndex()];
        Node* v = copiedGraph->m_nodes[edge->getEndNode()->getIndex()];
        copiedGraph->addEdge(u, v, edge->getCost());
    }

    return copiedGraph;
}

Graph* Graph::getInvertedGraph() const {
    if (!m_orientedGraph) {
        QMessageBox::warning(nullptr, "Warning", "Cannot invert an unoriented graph",
                             QMessageBox::Ok);
        return nullptr;
    }

    const auto invertedGraph = new Graph();

    invertedGraph->setAnimationsDisabled(m_animationsDisabled);
    invertedGraph->setAllowLoops(m_allowLoops);
    invertedGraph->setOrientedGraph(m_orientedGraph);

    for (auto node : m_nodes) {
        invertedGraph->addNode(node->pos());
    }

    invertedGraph->reserveEdges(m_edges.size());
    for (auto edge : m_edges) {
        Node* u = invertedGraph->m_nodes[edge->getStartNode()->getIndex()];
        Node* v = invertedGraph->m_nodes[edge->getEndNode()->getIndex()];

        invertedGraph->addEdge(v, u, edge->getCost());
        invertedGraph->m_adjacencyList[v].emplace(u);
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
            if (m_editingEnabled) {
                const auto scenePos = mapToScene(event->pos());
                addNode(scenePos);
            }
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void Graph::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete && m_editingEnabled) {
        removeSelectedNodes();
    } else if (event->key() == Qt::Key_Space) {
        emit spacePressed();
    } else if (event->key() == Qt::Key_Escape) {
        emit escapePressed();
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
    // m_nodeManager.addNode(pos.toPoint());

    /*const auto bounds = scene()->sceneRect();
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
    node->setAnimationDisabled(m_animationsDisabled);

    connect(node, &Node::markedForErasure, this, &Graph::onNodeMarkedForErasure);

    scene()->addItem(node);
    m_nodes.push_back(node);

    m_edges.reserve(m_nodes.size() * m_nodes.size());*/
}

void Graph::removeNodeConnections(Node* node) {
    removeEdgesConnectedToNode(node);
    m_adjacencyList.erase(node);

    for (auto& [_, list] : m_adjacencyList) {
        list.erase(node);
    }
}

void Graph::removeSelectedNodes() {
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

void Graph::removeAllNodes() {
    for (auto node : m_nodes) {
        removeNodeConnections(node);
        node->markForErasure();
    }

    m_nodes.clear();
}

void Graph::onNodeMarkedForErasure(Node* node) {
    scene()->removeItem(node);
    node->deleteLater();
}

void Graph::addEdge(Node* a, Node* b, int cost) {
    /*const auto edge = new Edge(a, b, cost);
    edge->setParent(this);
    edge->setUnorientedEdge(!m_orientedGraph);

    connect(edge, &Edge::markedForErasure, this, &Graph::onEdgeMarkedForErasure);

    m_edges.push_back(edge);
    scene()->addItem(edge);*/
}

size_t Graph::getMaxEdgesCount() {
    const size_t n = getNodesCount();
    if (m_orientedGraph) {
        return m_allowLoops ? n * n : n * (n - 1);
    }

    return n * (n - 1) / 2;
}

void Graph::reserveEdges(size_t edges) { m_nodeManager.reserveEdges(edges); }

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
