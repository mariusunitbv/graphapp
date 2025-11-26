#include <pch.h>

#include "Graph.h"

#include "algorithm_label/algorithm_label.h"

Graph::Graph(QWidget* parent) : QGraphicsView(parent), m_randomEngine(std::random_device()()) {}

Graph::~Graph() {}

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

void Graph::genericTraversal(Node* start) {
    if (!openStepDurationDialog()) {
        return;
    }

    markNodesAsUnvisited();

    enum NodeState : uint8_t { UNVISITED, VISITED, ANALYZED };

    size_t k = 1;
    std::vector<size_t> parents(m_nodes.size(), -1);
    std::vector<size_t> orders(m_nodes.size(), -1);
    std::vector<NodeState> visited(m_nodes.size(), UNVISITED);

    std::vector<Node*> nodesVector;

    const auto uText = AlgorithmLabel(scene(), mapToScene(10, 10), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == UNVISITED) {
                list << QString::number(i);
            }
        }

        label->setText("U = { " + list.join(", ") + " }");
    });

    const auto vText = AlgorithmLabel(scene(), mapToScene(10, 30), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == VISITED) {
                list << QString::number(i);
            }
        }

        label->setText("V = { " + list.join(", ") + " }");
    });

    const auto wText = AlgorithmLabel(scene(), mapToScene(10, 50), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == ANALYZED) {
                list << QString::number(i);
            }
        }

        label->setText("W = { " + list.join(", ") + " }");
    });

    const auto pText = AlgorithmLabel(scene(), mapToScene(10, 100), [&](auto label) {
        QStringList list;
        for (auto parent : parents) {
            list << (parent == -1 ? "-" : QString::number(parent));
        }

        label->setText("p = [ " + list.join(", ") + " ]");
    });

    const auto oText = AlgorithmLabel(scene(), mapToScene(10, 120), [&](auto label) {
        QStringList list;
        for (auto order : orders) {
            list << (order == -1 ? "-" : QString::number(order));
        }

        label->setText("o = [ " + list.join(", ") + " ]");
    });

    visited[start->getIndex()] = VISITED;
    orders[start->getIndex()] = 1;

    uText.compute();
    vText.compute();
    wText.compute();
    pText.compute();
    oText.compute();

    nodesVector.push_back(start);

    size_t i = 0;
    while (i < nodesVector.size()) {
        auto x = nodesVector[i];
        x->markCurrentlyAnalyzed();
        waitForStepDuration();

        for (auto y : m_adjacencyList[x]) {
            if (visited[y->getIndex()] == UNVISITED) {
                y->markVisited(x);

                visited[y->getIndex()] = VISITED;
                uText.compute();
                vText.compute();

                parents[y->getIndex()] = x->getIndex();
                pText.compute();

                orders[y->getIndex()] = ++k;
                oText.compute();

                waitForStepDuration();

                nodesVector.push_back(y);
            }
        }

        x->markAnalyzed(x);

        visited[x->getIndex()] = ANALYZED;
        vText.compute();
        wText.compute();

        waitForStepDuration();

        ++i;
        if (nodesVector.begin() + i != nodesVector.end()) {
            std::shuffle(nodesVector.begin() + i, nodesVector.end(), m_randomEngine);
        }
    }

    QMessageBox::information(nullptr, "Traversal Complete", "The traversal has completed.");

    unmarkNodes();
}

void Graph::genericTotalTraversal(Node* start) {
    if (!openStepDurationDialog()) {
        return;
    }

    markNodesAsUnvisited();

    enum NodeState : uint8_t { UNVISITED, VISITED, ANALYZED };

    size_t k = 1;
    std::vector<size_t> parents(m_nodes.size(), -1);
    std::vector<size_t> orders(m_nodes.size(), -1);
    std::vector<NodeState> visited(m_nodes.size(), UNVISITED);

    std::vector<Node*> nodesVector;

    const auto uText = AlgorithmLabel(scene(), mapToScene(10, 10), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == UNVISITED) {
                list << QString::number(i);
            }
        }

        label->setText("U = { " + list.join(", ") + " }");
    });

    const auto vText = AlgorithmLabel(scene(), mapToScene(10, 30), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == VISITED) {
                list << QString::number(i);
            }
        }

        label->setText("V = { " + list.join(", ") + " }");
    });

    const auto wText = AlgorithmLabel(scene(), mapToScene(10, 50), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == ANALYZED) {
                list << QString::number(i);
            }
        }

        label->setText("W = { " + list.join(", ") + " }");
    });

    const auto pText = AlgorithmLabel(scene(), mapToScene(10, 100), [&](auto label) {
        QStringList list;
        for (auto parent : parents) {
            list << (parent == -1 ? "-" : QString::number(parent));
        }

        label->setText("p = [ " + list.join(", ") + " ]");
    });

    const auto oText = AlgorithmLabel(scene(), mapToScene(10, 120), [&](auto label) {
        QStringList list;
        for (auto order : orders) {
            list << (order == -1 ? "-" : QString::number(order));
        }

        label->setText("o = [ " + list.join(", ") + " ]");
    });

    visited[start->getIndex()] = VISITED;
    orders[start->getIndex()] = 1;

    uText.compute();
    vText.compute();
    wText.compute();
    pText.compute();
    oText.compute();

    nodesVector.push_back(start);

    size_t i = 0;
    while (true) {
        while (i < nodesVector.size()) {
            auto x = nodesVector[i];
            x->markCurrentlyAnalyzed();
            waitForStepDuration();

            for (auto y : m_adjacencyList[x]) {
                if (visited[y->getIndex()] == UNVISITED) {
                    y->markVisited(x);

                    visited[y->getIndex()] = VISITED;
                    uText.compute();
                    vText.compute();

                    parents[y->getIndex()] = x->getIndex();
                    pText.compute();

                    orders[y->getIndex()] = ++k;
                    oText.compute();

                    waitForStepDuration();

                    nodesVector.push_back(y);
                }
            }

            x->markAnalyzed(x);

            visited[x->getIndex()] = ANALYZED;
            vText.compute();
            wText.compute();

            waitForStepDuration();

            ++i;
            if (nodesVector.begin() + i != nodesVector.end()) {
                std::shuffle(nodesVector.begin() + i, nodesVector.end(), m_randomEngine);
            }
        }

        bool finished = true;
        for (auto node : m_nodes) {
            if (visited[node->getIndex()] == UNVISITED) {
                visited[node->getIndex()] = VISITED;
                orders[node->getIndex()] = ++k;

                uText.compute();
                vText.compute();
                oText.compute();

                nodesVector.push_back(node);
                finished = false;

                break;
            }
        }

        if (finished) {
            break;
        }
    }

    QMessageBox::information(nullptr, "Traversal Complete", "The traversal has completed.");

    unmarkNodes();
}

void Graph::path(Node* start) {
    bool ok;
    int destinationNodeIndex = QInputDialog::getInt(nullptr, "Tracked node", "Enter node:", 0, 0,
                                                    static_cast<int>(m_nodes.size() - 1), 1, &ok);

    if (!ok || !openStepDurationDialog()) {
        return;
    }

    markNodesAsUnvisited();

    enum NodeState : uint8_t { UNVISITED, VISITED, ANALYZED };

    size_t k = 1;
    std::vector<size_t> parents(m_nodes.size(), -1);
    std::vector<size_t> orders(m_nodes.size(), -1);
    std::vector<NodeState> visited(m_nodes.size(), UNVISITED);

    std::vector<Node*> nodesVector;

    const auto uText = AlgorithmLabel(scene(), mapToScene(10, 10), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == UNVISITED) {
                list << QString::number(i);
            }
        }

        label->setText("U = { " + list.join(", ") + " }");
    });

    const auto vText = AlgorithmLabel(scene(), mapToScene(10, 30), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == VISITED) {
                list << QString::number(i);
            }
        }

        label->setText("V = { " + list.join(", ") + " }");
    });

    const auto wText = AlgorithmLabel(scene(), mapToScene(10, 50), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == ANALYZED) {
                list << QString::number(i);
            }
        }

        label->setText("W = { " + list.join(", ") + " }");
    });

    const auto pText = AlgorithmLabel(scene(), mapToScene(10, 100), [&](auto label) {
        QStringList list;
        for (auto parent : parents) {
            list << (parent == -1 ? "-" : QString::number(parent));
        }

        label->setText("p = [ " + list.join(", ") + " ]");
    });

    const auto oText = AlgorithmLabel(scene(), mapToScene(10, 120), [&](auto label) {
        QStringList list;
        for (auto order : orders) {
            list << (order == -1 ? "-" : QString::number(order));
        }

        label->setText("o = [ " + list.join(", ") + " ]");
    });

    visited[start->getIndex()] = VISITED;
    orders[start->getIndex()] = 1;

    nodesVector.push_back(start);

    size_t i = 0;
    while (i < nodesVector.size()) {
        auto x = nodesVector[i];
        x->markAvailableInPathFinding();

        for (auto y : m_adjacencyList[x]) {
            if (visited[y->getIndex()] == UNVISITED) {
                y->markAvailableInPathFinding();

                visited[y->getIndex()] = VISITED;
                parents[y->getIndex()] = x->getIndex();
                orders[y->getIndex()] = ++k;

                nodesVector.push_back(y);
            }
        }

        visited[x->getIndex()] = ANALYZED;
        uText.compute();
        vText.compute();
        wText.compute();
        pText.compute();
        oText.compute();

        ++i;
        if (nodesVector.begin() + i != nodesVector.end()) {
            std::shuffle(nodesVector.begin() + i, nodesVector.end(), m_randomEngine);
        }
    }

    if (visited[destinationNodeIndex] == UNVISITED) {
        QMessageBox::information(nullptr, "Pathing",
                                 "The destination node is unreachable from the start node.");
        unmarkNodes();
        return;
    }

    waitForStepDuration();

    auto y = m_nodes[destinationNodeIndex];

    y->markPath(nullptr);
    waitForStepDuration();

    while (parents[y->getIndex()] != -1) {
        auto x = m_nodes[parents[y->getIndex()]];

        x->markPath(y);
        waitForStepDuration();

        y = x;
    }

    QMessageBox::information(nullptr, "Pathing", "Path has finished");

    unmarkNodes();
}

void Graph::breadthFirstSearch(Node* start) {
    if (!openStepDurationDialog()) {
        return;
    }

    markNodesAsUnvisited();

    enum NodeState : uint8_t { UNVISITED, VISITED, ANALYZED };

    std::vector<size_t> parents(m_nodes.size(), -1);
    std::vector<size_t> lengths(m_nodes.size(), -1);
    std::vector<NodeState> visited(m_nodes.size(), UNVISITED);

    std::queue<Node*> nodesQueue;

    const auto uText = AlgorithmLabel(scene(), mapToScene(10, 10), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == UNVISITED) {
                list << QString::number(i);
            }
        }

        label->setText("U = { " + list.join(", ") + " }");
    });

    const auto vText = AlgorithmLabel(scene(), mapToScene(10, 30), [&](auto label) {
        auto queue = nodesQueue;

        QStringList list;
        while (!queue.empty()) {
            list << QString::number(queue.front()->getIndex());
            queue.pop();
        }

        label->setText("V = { " + list.join(", ") + " }");
    });

    const auto wText = AlgorithmLabel(scene(), mapToScene(10, 50), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == ANALYZED) {
                list << QString::number(i);
            }
        }

        label->setText("W = { " + list.join(", ") + " }");
    });

    const auto pText = AlgorithmLabel(scene(), mapToScene(10, 100), [&](auto label) {
        QStringList list;
        for (auto parent : parents) {
            list << (parent == -1 ? "-" : QString::number(parent));
        }

        label->setText("p = [ " + list.join(", ") + " ]");
    });

    const auto lText = AlgorithmLabel(scene(), mapToScene(10, 130), [&](auto label) {
        QStringList list;
        for (auto length : lengths) {
            list << (length == -1 ? "-" : QString::number(length));
        }

        label->setText("l = [ " + list.join(", ") + " ]");
    });

    visited[start->getIndex()] = VISITED;
    lengths[start->getIndex()] = 0;

    nodesQueue.push(start);

    uText.compute();
    vText.compute();
    wText.compute();
    pText.compute();
    lText.compute();

    while (!nodesQueue.empty()) {
        auto x = nodesQueue.front();
        x->markCurrentlyAnalyzed();
        waitForStepDuration();

        for (auto y : m_adjacencyList[x]) {
            if (visited[y->getIndex()] == UNVISITED) {
                y->markVisited(x);

                nodesQueue.push(y);

                visited[y->getIndex()] = VISITED;
                uText.compute();
                vText.compute();

                parents[y->getIndex()] = x->getIndex();
                pText.compute();

                lengths[y->getIndex()] = lengths[x->getIndex()] + 1;
                lText.compute();

                waitForStepDuration();
            }
        }

        x->markAnalyzed(x);
        nodesQueue.pop();

        visited[x->getIndex()] = ANALYZED;
        vText.compute();
        wText.compute();

        waitForStepDuration();
    }

    QMessageBox::information(nullptr, "Traversal Complete", "The traversal has completed.");

    unmarkNodes();
}

void Graph::depthFirstSearch(Node* start) {
    if (!openStepDurationDialog()) {
        return;
    }

    markNodesAsUnvisited();

    enum NodeState : uint8_t { UNVISITED, VISITED, ANALYZED };

    size_t t = 1;
    std::vector<size_t> parents(m_nodes.size(), -1);
    std::vector<size_t> discoveryTimes(m_nodes.size(), -1);
    std::vector<size_t> analyzedTimes(m_nodes.size(), -1);
    std::vector<NodeState> visited(m_nodes.size(), UNVISITED);

    std::stack<Node*> nodesStack;

    const auto uText = AlgorithmLabel(scene(), mapToScene(10, 10), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == UNVISITED) {
                list << QString::number(i);
            }
        }

        label->setText("U = { " + list.join(", ") + " }");
    });

    const auto vText = AlgorithmLabel(scene(), mapToScene(10, 30), [&](auto label) {
        auto stack = nodesStack;

        QStringList list;
        while (!stack.empty()) {
            list << QString::number(stack.top()->getIndex());
            stack.pop();
        }

        label->setText("V = { " + list.join(", ") + " }");
    });

    const auto wText = AlgorithmLabel(scene(), mapToScene(10, 50), [&](auto label) {
        QStringList list;
        for (size_t i = 0; i < visited.size(); ++i) {
            if (visited[i] == ANALYZED) {
                list << QString::number(i);
            }
        }

        label->setText("W = { " + list.join(", ") + " }");
    });

    const auto pText = AlgorithmLabel(scene(), mapToScene(10, 100), [&](auto label) {
        QStringList list;
        for (auto parent : parents) {
            list << (parent == -1 ? "-" : QString::number(parent));
        }

        label->setText("p = [ " + list.join(", ") + " ]");
    });

    const auto t1Text = AlgorithmLabel(scene(), mapToScene(10, 120), [&](auto label) {
        QStringList list;
        for (auto discoveryTime : discoveryTimes) {
            list << (discoveryTime == -1 ? "-" : QString::number(discoveryTime));
        }

        label->setText("t1 = [ " + list.join(", ") + " ]");
    });

    const auto t2Text = AlgorithmLabel(scene(), mapToScene(10, 140), [&](auto label) {
        QStringList list;
        for (auto analyzedTime : analyzedTimes) {
            list << (analyzedTime == -1 ? "-" : QString::number(analyzedTime));
        }

        label->setText("t2 = [ " + list.join(", ") + " ]");
    });

    visited[start->getIndex()] = VISITED;
    discoveryTimes[start->getIndex()] = 1;

    nodesStack.push(start);

    uText.compute();
    vText.compute();
    wText.compute();
    pText.compute();
    t1Text.compute();
    t2Text.compute();

    while (!nodesStack.empty()) {
        auto x = nodesStack.top();
        x->markCurrentlyAnalyzed();
        waitForStepDuration();

        bool visitedAllNeighbours = true;
        for (auto y : m_adjacencyList[x]) {
            if (visited[y->getIndex()] == UNVISITED) {
                y->markVisited(x);

                nodesStack.push(y);

                visited[y->getIndex()] = VISITED;
                uText.compute();
                vText.compute();

                parents[y->getIndex()] = x->getIndex();
                pText.compute();

                discoveryTimes[y->getIndex()] = ++t;
                t1Text.compute();

                waitForStepDuration();

                visitedAllNeighbours = false;
                break;
            }
        }

        if (!visitedAllNeighbours) {
            x->markVisitedButNotAnalyzedAnymore();
            continue;
        }

        x->markAnalyzed(x);
        nodesStack.pop();

        visited[x->getIndex()] = ANALYZED;
        vText.compute();
        wText.compute();

        analyzedTimes[x->getIndex()] = ++t;
        t2Text.compute();

        waitForStepDuration();
    }

    QMessageBox::information(nullptr, "Traversal Complete", "The traversal has completed.");

    unmarkNodes();
}

size_t Graph::getNodesCount() { return m_nodes.size(); }

int Graph::getZoomPercentage() { return static_cast<int>(m_currentZoomScale * 100); }

void Graph::resetGraph() {}

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
        if (m_isSelecting || m_isDragging) {
            setDragMode(QGraphicsView::NoDrag);
            m_isSelecting = m_isDragging = false;
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
    }

    QGraphicsView::keyPressEvent(event);
}

void Graph::addNode(const QPointF& pos) {
    Node* node = new Node(m_nodes.size());
    node->setPos(pos);

    const auto& nodeRect = node->sceneBoundingRect();
    if (!scene()->sceneRect().contains(nodeRect)) {
        delete node;
        return;
    }

    QPainterPath circlePath;
    circlePath.addEllipse(nodeRect);

    const auto collisions = scene()->items(circlePath, Qt::IntersectsItemShape);
    const bool collides = [&]() {
        for (auto item : collisions) {
            if (item->type() == Node::NodeType) {
                return true;
            }
        }

        return false;
    }();

    if (collides) {
        delete node;
        return;
    }

    scene()->addItem(node);
    m_nodes.push_back(node);

    const auto anim = new QPropertyAnimation(node, "radius");
    anim->setStartValue(1.);
    anim->setEndValue(Node::k_fullRadius);
    anim->setDuration(500);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Graph::removeNode(Node* node) {
    removeEdgesConnectedToNode(node);

    scene()->removeItem(node);
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
            removeNode(node);
            delete node;

            it = m_nodes.erase(it);
        } else {
            node->setIndex(index++);
            ++it;
        }
    }
}

void Graph::addEdge(Node* a, Node* b, int cost) {
    const auto edge = new Edge(a, b, cost);

    m_edges.push_back(edge);
    scene()->addItem(edge);

    const auto anim = new QPropertyAnimation(edge, "progress");
    anim->setStartValue(0.);
    anim->setEndValue(1.);
    anim->setDuration(500);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Graph::removeEdgesConnectedToNode(Node* node) {
    for (auto it = m_edges.begin(); it != m_edges.end();) {
        Edge* edge = *it;
        if (edge->connectsNode(node)) {
            scene()->removeItem(edge);
            delete edge;

            it = m_edges.erase(it);
        } else {
            ++it;
        }
    }
}

void Graph::resetAdjacencyList() {
    for (auto edge : m_edges) {
        scene()->removeItem(edge);
        delete edge;
    }

    m_edges.clear();
    m_adjacencyList.clear();
}

void Graph::markNodesAsUnvisited() {
    for (auto node : m_nodes) {
        if (node->isSelected()) {
            node->setSelected(false);
        }

        node->markUnvisited();
    }
}

void Graph::unmarkNodes() {
    for (auto node : m_nodes) {
        if (node->isSelected()) {
            node->setSelected(false);
        }

        node->unmark();
    }
}

bool Graph::openStepDurationDialog() {
    bool ok = false;
    int stepDelay = QInputDialog::getInt(nullptr, "Step Delay", "Enter step delay (ms):", 500, 0,
                                         5000, 50, &ok);

    if (ok) {
        m_stepDuration = stepDelay;
        return true;
    }

    return false;
}

void Graph::waitForStepDuration() {
    if (m_stepDuration <= 0) {
        return;
    }

    scene()->update();
    QCoreApplication::processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(m_stepDuration));
}
