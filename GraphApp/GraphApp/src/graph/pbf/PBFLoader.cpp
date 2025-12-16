#include <pch.h>

#include "PBFLoader.h"

PBFLoader::PBFLoader(GraphManager* graphManager, const QString& pbfFile)
    : m_graphManager(graphManager) {
    m_pbfPath = pbfFile.toStdString();
}

void PBFLoader::tryLoad() {
    bool ok = false;
    const auto accuracy = QInputDialog::getDouble(
        nullptr, "Accuracy",
        "A smaller accuracy will merge near nodes.\nBut a bigger accuracy "
        "may crash the app if loading a bigger map!\nEntered accuracy (1 -> 25):",
        5, 1, 25, 1, &ok);
    if (!ok) {
        return;
    }

    m_accuracy = NodeData::k_radius * (5 / accuracy);
    m_loadingScreen = new LoadingScreen("Loading .pbf file");
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
            QString("An error occurred while loading the PBF file:\n%1").arg(ex.what()),
            QMessageBox::Ok);
    }

    m_graphManager->setCollisionsCheckEnabled(true);
}

QPointF PBFLoader::toMercator(qreal lat, qreal lon) const {
    const auto x = EARTH_RADIUS * qDegreesToRadians(lon);
    const auto y = EARTH_RADIUS * std::log(std::tan(M_PI / 4. + qDegreesToRadians(lat) / 2.));

    return {x, y};
}

QPoint PBFLoader::mercatorToGraphPosition(const QPointF& mercatorPos) const {
    const auto graphSize = m_graphManager->boundingRect().size();
    const auto padding = NodeData::k_radius + 1;

    const auto availableWidth = graphSize.width() - 2 * padding;
    const auto availableHeight = graphSize.height() - 2 * padding;

    const auto dataWidth = m_maxX - m_minX;
    const auto dataHeight = m_maxY - m_minY;

    const auto dataAspectRatio = dataWidth / dataHeight;
    const auto canvasAspectRatio = availableWidth / availableHeight;

    qreal finalWidth, finalHeight, offsetX = 0, offsetY = 0;
    if (dataAspectRatio > canvasAspectRatio) {
        finalWidth = availableWidth;
        finalHeight = availableWidth / dataAspectRatio;
        offsetY = (availableHeight - finalHeight) / 2.;
    } else {
        finalHeight = availableHeight;
        finalWidth = availableHeight * dataAspectRatio;
        offsetX = (availableWidth - finalWidth) / 2.;
    }

    const auto nx = (mercatorPos.x() - m_minX) / dataWidth;
    const auto ny = (mercatorPos.y() - m_minY) / dataHeight;

    const auto x = padding + offsetX + nx * finalWidth;
    const auto y = padding + offsetY + (1. - ny) * finalHeight;

    return QPointF{x, y}.toPoint();
}

qreal PBFLoader::haversineDistanceKm(qreal lat1, qreal lat2, qreal lon1, qreal lon2) const {
    const auto phi1 = qDegreesToRadians(lat1);
    const auto phi2 = qDegreesToRadians(lat2);
    const auto dPhi = qDegreesToRadians(lat2 - lat1);
    const auto dLambda = qDegreesToRadians(lon2 - lon1);

    const auto havTheta = qSin(dPhi / 2) * qSin(dPhi / 2) +
                          qCos(phi1) * qCos(phi2) * qSin(dLambda / 2) * qSin(dLambda / 2);
    const auto theta = 2 * qAsin(qSqrt(havTheta));

    return EARTH_RADIUS * theta;
}

void PBFLoader::parseAndComputeBounds() {
    using namespace osmium;

    index::map::SparseMemArray<unsigned_object_id_type, Location> index;
    handler::NodeLocationsForWays locationHandler(index);

    size_t parsedNodes{};
    io::Reader reader(m_pbfPath, osm_entity_bits::node | osm_entity_bits::way);

    while (auto buffer = reader.read()) {
        apply(buffer, locationHandler);
        for (const auto& way : buffer.select<Way>()) {
            if (!way.tags().get_value_by_key("highway")) {
                continue;
            }

            WayData wayData;
            wayData.m_oneWay = [&way]() {
                auto oneWay = way.tags().get_value_by_key("oneway");
                if (!oneWay) {
                    return false;
                }

                return std::strcmp(oneWay, "yes") == 0 || std::strcmp(oneWay, "true") == 0 ||
                       std::strcmp(oneWay, "1") == 0;
            }();

            auto& mercatorPoints = wayData.m_mercatorPoints;
            mercatorPoints.reserve(way.nodes().size());

            qreal lastLon{}, lastLat{};
            for (const auto& nodeRef : way.nodes()) {
                const auto loc = index.get(nodeRef.ref());
                const auto lon = loc.lon(), lat = loc.lat();
                const auto mercatorPos = toMercator(lat, lon);

                if (mercatorPoints.empty()) {
                    mercatorPoints.emplace_back(mercatorPos, 0);
                } else {
                    mercatorPoints.emplace_back(mercatorPos,
                                                haversineDistanceKm(lastLat, lat, lastLon, lon));
                }

                if (++parsedNodes % 100000 == 0) {
                    m_loadingScreen->setText(QString("Parsed %1 nodes").arg(parsedNodes));
                }

                m_minX = std::min(m_minX, mercatorPos.x());
                m_maxX = std::max(m_maxX, mercatorPos.x());
                m_minY = std::min(m_minY, mercatorPos.y());
                m_maxY = std::max(m_maxY, mercatorPos.y());

                lastLon = lon;
                lastLat = lat;
            }

            if (mercatorPoints.size() > 1) {
                m_ways.push_back(std::move(wayData));
            }
        }
    }
}

void PBFLoader::addNodesToGraph() {
    size_t addedNodes{};

    for (const auto& [points, _] : m_ways) {
        for (const auto& [mercatorPos, _] : points) {
            const auto screenPos = mercatorToGraphPosition(mercatorPos);

            if (m_screenToNodes.contains(screenPos)) {
                continue;
            }

            const auto nearestNode = m_graphManager->getNode(screenPos, m_accuracy);
            if (nearestNode.has_value()) {
                m_screenToNodes.emplace(screenPos, nearestNode.value());
                continue;
            }

            if (!m_graphManager->addNode(screenPos)) {
                throw std::runtime_error(
                    "Failed to add node to the graph.\nMap contains too many nodes!");
            }

            if (++addedNodes % 100000 == 0) {
                m_loadingScreen->setText(QString("Added %1 nodes").arg(addedNodes));
            }

            m_screenToNodes.emplace(screenPos, m_graphManager->getNodesCount() - 1);
        }
    }
}

void PBFLoader::connectNodes() {
    size_t connectedWays{};
    m_graphManager->resizeAdjacencyMatrix(m_graphManager->getNodesCount());

    for (const auto& [points, oneWay] : m_ways) {
        NodeIndex_t prevNodeIndex = INVALID_NODE;
        for (const auto& [mercatorPos, distance] : points) {
            const auto screenPos = mercatorToGraphPosition(mercatorPos);

            const auto nodeIndex = m_screenToNodes.value(screenPos, INVALID_NODE);
            if (nodeIndex == INVALID_NODE) {
                throw std::runtime_error("Failed to find node for way connection.");
            }

            if (prevNodeIndex == INVALID_NODE || prevNodeIndex == nodeIndex) {
                prevNodeIndex = nodeIndex;
                continue;
            }

            m_graphManager->addEdge(prevNodeIndex, nodeIndex, distance);
            if (!oneWay) {
                m_graphManager->addEdge(nodeIndex, prevNodeIndex, distance);
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
