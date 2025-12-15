#include <pch.h>

#include "GeoJSONLoader.h"

GeoJSONLoader::GeoJSONLoader(GraphManager* graphManager, const QString& geoJsonFile)
    : m_graphManager(graphManager) {
    m_jsonPath = geoJsonFile.toStdString();
}

void GeoJSONLoader::tryLoad() {
    bool ok = false;
    const auto accuracy = QInputDialog::getDouble(
        nullptr, "Accuracy",
        "A smaller accuracy will merge near nodes.\nBut a bigger accuracy "
        "may crash the app if loading a bigger map!\nEntered accuracy (1-20):",
        5, 1, 20, 1, &ok);
    if (!ok) {
        return;
    }

    m_accuracy = NodeData::k_radius * (5 / accuracy);
    m_loadingScreen = new LoadingScreen("Loading .geojson file\nand computing bounds");
    m_loadingScreen->forceShow();

    try {
        m_graphManager->reset();
        m_graphManager->setAllowEditing(false);
        m_graphManager->setCollisionsCheckEnabled(false);

        parseAndComputeBounds();
        addNodesToGraph();
        connectNodes();
    } catch (const std::exception& ex) {
        m_loadingScreen->close();

        m_graphManager->reset();
        m_graphManager->setAllowEditing(true);

        QMessageBox::warning(
            nullptr, "Error",
            QString("An error occurred while loading the GeoJSON file:\n%1").arg(ex.what()),
            QMessageBox::Ok);
    }

    m_graphManager->setCollisionsCheckEnabled(true);
}

QPointF GeoJSONLoader::toMercator(qreal lat, qreal lon) const {
    static constexpr auto R = 6378137.0;

    const auto x = R * lon * M_PI / 180.0;
    const auto y = R * std::log(std::tan(M_PI / 4.0 + lat * M_PI / 360.0));

    return {x, y};
}

QPoint GeoJSONLoader::mercatorToGraphPosition(const QPointF& mercatorPos) const {
    const auto graphSize = m_graphManager->boundingRect().size();
    const auto padding = NodeData::k_radius + 1;

    const auto nx = (mercatorPos.x() - m_minX) / (m_maxX - m_minX);
    const auto ny = (mercatorPos.y() - m_minY) / (m_maxY - m_minY);

    const auto x = padding + nx * (graphSize.width() - 2 * padding);
    const auto y = padding + (1.0 - ny) * (graphSize.height() - 2 * padding);

    return QPointF{x, y}.toPoint();
}

void GeoJSONLoader::parseAndComputeBounds() {
    using namespace simdjson;

    ondemand::parser parser;
    auto json = padded_string::load(m_jsonPath);
    ondemand::document doc = parser.iterate(json);

    ondemand::array features = doc["features"];
    for (auto feature : features) {
        ondemand::object properties = feature["properties"];
        const auto oneWay = [&properties]() {
            auto field = properties.find_field("oneway");
            if (field.error() == SUCCESS) {
                std::string_view value = field.value().get_string();
                if (value == "yes" || value == "1") {
                    return true;
                }
            }

            return false;
        }();

        ondemand::object geometry = feature["geometry"];
        std::string_view type = geometry["type"].get_string();
        if (type != "LineString") {
            continue;
        }

        WayData wayData;
        wayData.m_oneWay = oneWay;

        ondemand::array coords = geometry["coordinates"];
        wayData.m_mercatorPoints.reserve(coords.count_elements());

        for (auto coord : coords) {
            auto it = coord.begin();
            const qreal lon = *it, lat = *(++it);
            const auto mercatorPos = toMercator(lat, lon);

            wayData.m_mercatorPoints.push_back(mercatorPos);

            m_minX = std::min(m_minX, mercatorPos.x());
            m_maxX = std::max(m_maxX, mercatorPos.x());
            m_minY = std::min(m_minY, mercatorPos.y());
            m_maxY = std::max(m_maxY, mercatorPos.y());
        }

        m_ways.push_back(std::move(wayData));
    }
}

void GeoJSONLoader::addNodesToGraph() {
    size_t addedNodes{};

    for (const auto& [points, oneWay] : m_ways) {
        for (const auto& mercatorPos : points) {
            const auto screenPos = mercatorToGraphPosition(mercatorPos);

            const auto nearestNode = m_graphManager->getNode(screenPos, m_accuracy);
            if (nearestNode.has_value()) {
                continue;
            }

            if (!m_graphManager->addNode(screenPos)) {
                throw std::runtime_error(
                    "Failed to add node to the graph.\nMap contains too many nodes!");
            }

            if (++addedNodes % 50000 == 0) {
                m_loadingScreen->setText(QString("Added %1 nodes").arg(addedNodes));
            }
        }
    }
}

void GeoJSONLoader::connectNodes() {
    size_t connectedWays{};
    m_graphManager->resizeAdjacencyMatrix(m_graphManager->getNodesCount());

    for (const auto& [points, oneWay] : m_ways) {
        NodeIndex_t prevNodeIndex = INVALID_NODE;
        for (const auto& mercatorPos : points) {
            const auto screenPos = mercatorToGraphPosition(mercatorPos);

            const auto nodeIndexOpt = m_graphManager->getNode(screenPos, m_accuracy);
            if (!nodeIndexOpt.has_value()) {
                throw std::runtime_error("Internal error: Node not found in graph.");
            }

            const auto nodeIndex = nodeIndexOpt.value();
            if (prevNodeIndex == INVALID_NODE || prevNodeIndex == nodeIndex) {
                prevNodeIndex = nodeIndex;
                continue;
            }

            m_graphManager->addEdge(prevNodeIndex, nodeIndex, 1);
            if (!oneWay) {
                m_graphManager->addEdge(nodeIndex, prevNodeIndex, 1);
            }

            prevNodeIndex = nodeIndex;
        }

        if (++connectedWays % 50000 == 0) {
            m_loadingScreen->setText(QString("Connected %1 ways").arg(connectedWays));
        }
    }

    m_loadingScreen->close();
    m_graphManager->buildFullEdgeCache();
}
