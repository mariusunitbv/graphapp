#include <pch.h>

#include "PBFLoader.h"

#include "../form/pbf_loader/PbfLoadSettings.h"

PBFLoader::PBFLoader(GraphManager* graphManager, const QString& pbfFile)
    : m_graphManager(graphManager) {
    m_pbfPath = pbfFile.toStdString();
}

void PBFLoader::tryLoad() {
    PbfLoadSettings settingsDialog;
    if (settingsDialog.exec() != QDialog::Accepted) {
        return;
    }

    m_shouldParseBoundaries = settingsDialog.parseBoundaries();

    m_parseMotorways = settingsDialog.parseMotorways();
    m_parseMotorwayLinks = settingsDialog.parseMotorwayLinks();

    m_parseTrunks = settingsDialog.parseTrunks();
    m_parseTrunkLinks = settingsDialog.parseTrunkLinks();

    m_parsePrimarys = settingsDialog.parsePrimarys();
    m_parsePrimaryLinks = settingsDialog.parsePrimaryLinks();

    m_parseSecondarys = settingsDialog.parseSecondarys();
    m_parseSecondaryLinks = settingsDialog.parseSecondaryLinks();

    m_parseTertiarys = settingsDialog.parseTertiarys();
    m_parseTertiaryLinks = settingsDialog.parseTertiaryLinks();

    m_parseUnclassifieds = settingsDialog.parseUnclassifieds();
    m_parseResidentials = settingsDialog.parseResidentials();
    m_parseLivingStreets = settingsDialog.parseLivingStreets();
    m_parseServices = settingsDialog.parseServices();
    m_parsePedestrians = settingsDialog.parsePedestrians();

    const auto mergeFactor = settingsDialog.getMergeFactor();
    const float accuracy = 9.f - mergeFactor;

    m_accuracy = mergeFactor == 0 ? 0.f : NodeData::k_radius / accuracy;
    m_loadingScreen = new LoadingScreen("Loading .pbf file");
    m_loadingScreen->forceShow();

    try {
        m_graphManager->reset();
        m_graphManager->setDrawNodesEnabled(false);
        m_graphManager->setAllowEditing(false);
        m_graphManager->setCollisionsCheckEnabled(false);
        m_graphManager->setOrientedGraph(true);

        checkIfNodeForWaysNeeded();
        parseAndComputeBounds();
        addNodesToGraph();
        connectNodes();
    } catch (const std::exception& ex) {
        m_loadingScreen->close();

        m_graphManager->reset();
        m_graphManager->setDrawNodesEnabled(true);
        m_graphManager->setAllowEditing(true);

        QMessageBox::warning(
            nullptr, "Error",
            QString("An error occurred while loading the PBF file:\n%1").arg(ex.what()),
            QMessageBox::Ok);
    }

    m_graphManager->setCollisionsCheckEnabled(true);
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

void PBFLoader::checkIfNodeForWaysNeeded() {
    using namespace osmium;

    io::Reader reader(m_pbfPath, osm_entity_bits::node | osm_entity_bits::way, io::read_meta::no);
    size_t waysChecked{};

    while (auto buffer = reader.read()) {
        for (const auto& way : buffer.select<Way>()) {
            if (waysChecked >= 50) {
                return;
            }

            const auto& nodes = way.nodes();
            for (const auto& node : nodes) {
                if (!node.location().valid()) {
                    m_nodeForWaysNeeded = true;
                    return;
                }
            }

            ++waysChecked;
        }
    }
}

void PBFLoader::parseAndComputeBounds() {
    using namespace osmium;

    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();

    index::map::FlexMem<unsigned_object_id_type, Location> index;
    handler::NodeLocationsForWays handler(index);
    io::Reader reader(m_pbfPath, osm_entity_bits::node | osm_entity_bits::way, io::read_meta::no);

    while (auto buffer = reader.read()) {
        if (m_nodeForWaysNeeded) {
            apply(buffer, handler);
        }

        for (const auto& way : buffer.select<Way>()) {
            const auto highwayKey = way.tags().get_value_by_key("highway");
            const auto boundaryKey = way.tags().get_value_by_key("boundary");

            if (!highwayKey && !boundaryKey) {
                continue;
            }

            if (boundaryKey && !m_shouldParseBoundaries) {
                continue;
            }

            if (highwayKey && shouldSkipHighway(highwayKey)) {
                continue;
            }

            const auto& nodes = way.nodes();
            if (nodes.size() < 2) {
                continue;
            }

            WayData wayData;

            const auto oneWay = way.tags().get_value_by_key("oneway");
            wayData.m_oneWay = oneWay && (*oneWay == 'y' || *oneWay == 't' || *oneWay == '1');

            auto& locations = wayData.m_locations;
            locations.reserve(nodes.size());

            for (const auto& node : nodes) {
                const auto loc = m_nodeForWaysNeeded ? index.get(node.ref()) : node.location();
                const auto mercatorPos = m_projection(loc);
                locations.push_back(loc);

                m_minX = std::min(m_minX, mercatorPos.x);
                m_maxX = std::max(m_maxX, mercatorPos.x);
                m_minY = std::min(m_minY, mercatorPos.y);
                m_maxY = std::max(m_maxY, mercatorPos.y);
            }

            m_ways.push_back(std::move(wayData));
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        if (elapsed.count() >= 2) {
            lastUpdate = now;
            m_loadingScreen->setText(QString("Parsed %1 ways").arg(m_ways.size()));
        }
    }
}

void PBFLoader::addNodesToGraph() {
    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();

    for (const auto& [points, _] : m_ways) {
        for (const auto loc : points) {
            const auto mercatorPosCoord = m_projection(loc);
            const QPointF mercatorPos{mercatorPosCoord.x, mercatorPosCoord.y};

            const auto screenPos = mercatorToGraphPosition(mercatorPos);
            if (m_screenToNodes.contains(screenPos)) {
                continue;
            }

            if (m_accuracy > 0.f) {
                const auto nearestNode = m_graphManager->getNode(screenPos, m_accuracy);
                if (nearestNode.has_value()) {
                    m_screenToNodes.emplace(screenPos, nearestNode.value());
                    continue;
                }
            }

            if (!m_graphManager->addNode(screenPos)) {
                throw std::runtime_error(
                    "Failed to add node to the graph.\nNode is out of bounds!");
            }

            m_screenToNodes.emplace(screenPos, m_graphManager->getNodesCount() - 1);
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        if (elapsed.count() >= 2) {
            lastUpdate = now;
            m_loadingScreen->setText(
                QString("Added %1 nodes").arg(m_graphManager->getNodesCount()));
        }
    }
}

void PBFLoader::connectNodes() {
    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();
    size_t connectedWays{};

    m_graphManager->resizeAdjacencyMatrix(m_graphManager->getNodesCount());

    for (const auto& [points, oneWay] : m_ways) {
        NodeIndex_t prevNodeIndex = INVALID_NODE;
        osmium::Location prevLocation;
        int64_t distance{};

        for (const auto loc : points) {
            const auto mercatorPosCoord = m_projection(loc);
            const QPointF mercatorPos{mercatorPosCoord.x, mercatorPosCoord.y};

            const auto screenPos = mercatorToGraphPosition(mercatorPos);
            const auto nodeIndex = m_screenToNodes.value(screenPos, INVALID_NODE);
            if (nodeIndex == INVALID_NODE) {
                throw std::runtime_error("Failed to find node for way connection.");
            }

            if (prevNodeIndex == INVALID_NODE) {
                prevNodeIndex = nodeIndex;
                prevLocation = loc;

                continue;
            }

            const auto haversineDist = osmium::geom::haversine::distance(prevLocation, loc);
            distance += static_cast<int64_t>(std::ceil(haversineDist));

            if (prevNodeIndex == nodeIndex) {
                prevLocation = loc;
                continue;
            }

            m_graphManager->addEdge(prevNodeIndex, nodeIndex, distance);
            if (!oneWay) {
                m_graphManager->addEdge(nodeIndex, prevNodeIndex, distance);
            }

            prevNodeIndex = nodeIndex;
            prevLocation = loc;
            distance = 0;
        }

        ++connectedWays;

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        if (elapsed.count() >= 2) {
            lastUpdate = now;
            m_loadingScreen->setText(QString("Connected %1 ways").arg(connectedWays));
        }
    }

    m_loadingScreen->close();
    m_graphManager->buildEdgeCache();
}

bool PBFLoader::shouldSkipHighway(const std::string_view highwayKey) const {
    const auto endsInInk = highwayKey.ends_with("ink");

    if (!m_parseMotorways && highwayKey.starts_with("mo") && !endsInInk) {
        return true;
    }

    if (!m_parseMotorwayLinks && highwayKey.starts_with("mo") && endsInInk) {
        return true;
    }

    if (!m_parseTrunks && highwayKey.starts_with("tru") && !endsInInk) {
        return true;
    }

    if (!m_parseTrunkLinks && highwayKey.starts_with("tru") && endsInInk) {
        return true;
    }

    if (!m_parsePrimarys && highwayKey.starts_with("pri") && !endsInInk) {
        return true;
    }

    if (!m_parsePrimaryLinks && highwayKey.starts_with("pri") && endsInInk) {
        return true;
    }

    if (!m_parseSecondarys && highwayKey.starts_with("sec") && !endsInInk) {
        return true;
    }

    if (!m_parseSecondaryLinks && highwayKey.starts_with("sec") && endsInInk) {
        return true;
    }

    if (!m_parseTertiarys && highwayKey.starts_with("ter") && !endsInInk) {
        return true;
    }

    if (!m_parseTertiaryLinks && highwayKey.starts_with("ter") && endsInInk) {
        return true;
    }

    if (!m_parseUnclassifieds && highwayKey.starts_with("unc")) {
        return true;
    }

    if (!m_parseResidentials && highwayKey.starts_with("res")) {
        return true;
    }

    if (!m_parseLivingStreets && highwayKey.starts_with("liv")) {
        return true;
    }

    if (!m_parseServices && highwayKey.starts_with("ser")) {
        return true;
    }

    if (!m_parsePedestrians && highwayKey.starts_with("ped")) {
        return true;
    }

    return false;
}
