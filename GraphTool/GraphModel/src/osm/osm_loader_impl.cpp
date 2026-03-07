module;
#include <pch.h>

module osm_loader;

import graph_model_defines;

#ifdef __EMSCRIPTEN__
OSMLoader::OSMLoader(GraphModel*, const std::string_view) {
    GAPP_THROW("OSM loading is not supported in WebAssembly builds");
}

void OSMLoader::tryLoad() { GAPP_THROW("OSM loading is not supported in WebAssembly builds"); }
#else
static constexpr auto BOUND_LIMIT = 500'000;
static constexpr auto graphSize =
    BoundingBox2D{-BOUND_LIMIT, -BOUND_LIMIT, BOUND_LIMIT, BOUND_LIMIT};
static constexpr auto ACCURACY = 0.03f;

OSMLoader::OSMLoader(GraphModel* model, const std::string_view osmFile)
    : m_model(model), m_osmPath(osmFile) {
    if (osmFile.empty()) {
        GAPP_THROW("OSM file path cannot be empty");
    }
}

void OSMLoader::tryLoad() {
    m_model->reserveArea(graphSize);

    checkIfNodeForWaysNeeded();
    getNeededNodes();
    parseAndComputeBounds();
    addNodesToGraph();
}

Vector2D OSMLoader::mercatorToWorld(const Vector2D& mercatorPos) const {
    float finalWidth, finalHeight, offsetX = 0, offsetY = 0;
    if (m_dataAspectRatio > m_canvasAspectRatio) {
        finalWidth = m_availableWidth;
        finalHeight = m_availableWidth / m_dataAspectRatio;
        offsetY = (m_availableHeight - finalHeight) * 0.5f;
    } else {
        finalHeight = m_availableHeight;
        finalWidth = m_availableHeight * m_dataAspectRatio;
        offsetX = (m_availableWidth - finalWidth) * 0.5f;
    }

    const auto nx = (mercatorPos.m_x - static_cast<float>(m_minX)) / m_dataWidth;
    const auto ny = (mercatorPos.m_y - static_cast<float>(m_minY)) / m_dataHeight;

    const auto x = -BOUND_LIMIT + NODE_RADIUS + offsetX + nx * finalWidth;
    const auto y = -BOUND_LIMIT + NODE_RADIUS + offsetY + (1.f - ny) * finalHeight;

    return {x, y};
}

void OSMLoader::checkIfNodeForWaysNeeded() {
    using namespace osmium;

    io::Reader reader(m_osmPath, osm_entity_bits::node | osm_entity_bits::way, io::read_meta::no);
    size_t nodesChecked{};

    while (auto buffer = reader.read()) {
        for (const auto& way : buffer.select<Way>()) {
            const auto& nodes = way.nodes();
            for (const auto& node : nodes) {
                if (nodesChecked >= 50) {
                    std::cout << "Node locations for ways are not needed.\n";
                    return;
                }

                if (!node.location().valid()) {
                    m_nodeForWaysNeeded = true;
                    std::cout << "Node locations for ways are needed.\n";
                    return;
                }

                ++nodesChecked;
            }
        }
    }
}

void OSMLoader::getNeededNodes() {
    if (!m_nodeForWaysNeeded) {
        return;
    }

    using namespace osmium;

    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();

    io::Reader reader(m_osmPath, osm_entity_bits::way, io::read_meta::no);
    while (auto buffer = reader.read()) {
        for (const auto& way : buffer.select<Way>()) {
            const auto highwayKey = way.tags().get_value_by_key("highway");
            const auto boundaryKey = way.tags().get_value_by_key("boundary");

            if (!highwayKey /*&& !boundaryKey*/) {
                continue;
            }

            const auto isRoad = [](const char* h) {
                return strcmp(h, "motorway") == 0 || strcmp(h, "trunk") == 0 ||
                       strcmp(h, "primary") == 0 || strcmp(h, "secondary") == 0 ||
                       strcmp(h, "tertiary") == 0 || strcmp(h, "unclassified") == 0 ||
                       strcmp(h, "residential") == 0 || strcmp(h, "motorway_link") == 0 ||
                       strcmp(h, "trunk_link") == 0 || strcmp(h, "primary_link") == 0 ||
                       strcmp(h, "secondary_link") == 0 || strcmp(h, "tertiary_link") == 0 ||
                       strcmp(h, "living_street") == 0 || strcmp(h, "service") == 0;
            };

            if (!isRoad(highwayKey)) {
                continue;
            }

            /*if (boundaryKey && !m_shouldParseBoundaries) {
                continue;
            }

            if (highwayKey && shouldSkipHighway(highwayKey)) {
                continue;
            }*/

            const auto& nodes = way.nodes();
            if (nodes.size() < 2) {
                continue;
            }

            ++m_parsedWayCount;
            m_parsedNodeCount += static_cast<uint32_t>(nodes.size());

            for (const auto& node : nodes) {
                m_nodesLocations.emplace(node.ref(), osmium::Location{});
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        if (elapsed.count() >= 2) {
            lastUpdate = now;
            if (m_parsedWayCount > 0) {
                std::cout << std::format("Found {} ways with {} nodes that need to be parsed...\n",
                                         m_parsedWayCount, m_parsedNodeCount);
            }
        }
    }

    m_nodesForWays.reserve(m_nodesLocations.size());
    m_waysMeta.reserve(m_parsedWayCount);

    std::cout << std::format(
        "Finished checking nodes for ways. Found {} ways with {} nodes that need to be parsed.\n",
        m_parsedWayCount, m_parsedNodeCount);
}

void OSMLoader::parseAndComputeBounds() {
    using namespace osmium;

    uint32_t parsedNodeLocations = 0;
    m_parsedWayCount = m_parsedNodeCount = 0;
    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();

    const auto readFlags =
        m_nodeForWaysNeeded ? osm_entity_bits::node | osm_entity_bits::way : osm_entity_bits::way;

    io::Reader reader(m_osmPath, readFlags, io::read_meta::no);
    while (auto buffer = reader.read()) {
        if (m_nodeForWaysNeeded) {
            for (const auto& node : buffer.select<osmium::Node>()) {
                auto it = m_nodesLocations.find(node.id());
                if (it == m_nodesLocations.end()) {
                    continue;
                }

                ++parsedNodeLocations;
                it->second = node.location();
            }

            const auto now = std::chrono::steady_clock::now();
            const auto duration =
                std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
            if (duration.count() >= 2) {
                lastUpdate = now;
                std::cout << std::format("Retrieved {}/{} nodes for ways...\n", parsedNodeLocations,
                                         m_nodesLocations.size());
            }
        }

        for (const auto& way : buffer.select<Way>()) {
            const auto highwayKey = way.tags().get_value_by_key("highway");
            const auto boundaryKey = way.tags().get_value_by_key("boundary");

            if (!highwayKey /* && !boundaryKey*/) {
                continue;
            }

            const auto isRoad = [](const char* h) {
                return strcmp(h, "motorway") == 0 || strcmp(h, "trunk") == 0 ||
                       strcmp(h, "primary") == 0 || strcmp(h, "secondary") == 0 ||
                       strcmp(h, "tertiary") == 0 || strcmp(h, "unclassified") == 0 ||
                       strcmp(h, "residential") == 0 || strcmp(h, "motorway_link") == 0 ||
                       strcmp(h, "trunk_link") == 0 || strcmp(h, "primary_link") == 0 ||
                       strcmp(h, "secondary_link") == 0 || strcmp(h, "tertiary_link") == 0 ||
                       strcmp(h, "living_street") == 0 || strcmp(h, "service") == 0;
            };

            if (!isRoad(highwayKey)) {
                continue;
            }

            /*if (boundaryKey && !m_shouldParseBoundaries) {
                continue;
            }

            if (highwayKey && shouldSkipHighway(highwayKey)) {
                continue;
            }*/

            const auto& nodes = way.nodes();
            if (nodes.size() < 2) {
                continue;
            }

            ++m_parsedWayCount;
            m_parsedNodeCount += static_cast<uint32_t>(nodes.size());

            const auto oneWay = way.tags().get_value_by_key("oneway");
            const auto isOneWay = oneWay && (*oneWay == 'y' || *oneWay == 't' || *oneWay == '1');
            const auto startIndex = m_nodesForWays.size();

            for (const auto& node : nodes) {
                const auto loc =
                    m_nodeForWaysNeeded ? m_nodesLocations.at(node.ref()) : node.location();

                m_nodesForWays.push_back(loc);

                const auto mercatorPos = m_projection(loc);
                m_minX = std::min(m_minX, mercatorPos.x);
                m_maxX = std::max(m_maxX, mercatorPos.x);
                m_minY = std::min(m_minY, mercatorPos.y);
                m_maxY = std::max(m_maxY, mercatorPos.y);
            }

            m_waysMeta.emplace_back(static_cast<uint32_t>(startIndex), isOneWay);
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        if (elapsed.count() >= 2) {
            lastUpdate = now;
            if (!m_waysMeta.empty()) {
                std::cout << std::format("Parsing {} ways with {} nodes...\n", m_parsedWayCount,
                                         m_parsedNodeCount);
            }
        }
    }

    std::cout << std::format("Finished parsing. Parsed {} ways with {} nodes.\n", m_parsedWayCount,
                             m_parsedNodeCount);

    m_availableWidth = graphSize.width() - NODE_DIAMETER;
    m_availableHeight = graphSize.height() - NODE_DIAMETER;
    m_canvasAspectRatio = m_availableWidth / m_availableHeight;

    m_dataWidth = static_cast<float>(m_maxX - m_minX);
    m_dataHeight = static_cast<float>(m_maxY - m_minY);
    m_dataAspectRatio = m_dataWidth / m_dataHeight;
}

void OSMLoader::addNodesToGraph() {
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastUpdate = startTime;

    for (size_t metaIndex = 0; metaIndex < m_waysMeta.size(); ++metaIndex) {
        const auto currentMeta = m_waysMeta[metaIndex];
        const auto metaLastNodeIndex = metaIndex + 1 == m_waysMeta.size()
                                           ? static_cast<uint32_t>(m_nodesForWays.size())
                                           : m_waysMeta[metaIndex + 1].m_startIndex;

        NodeIndex_t prevNodeIndex = INVALID_NODE;
        for (size_t currentNode = currentMeta.m_startIndex; currentNode < metaLastNodeIndex;
             ++currentNode) {
            const auto loc = m_nodesForWays[currentNode];

            const auto mercatorPosCoord = m_projection(loc);
            const Vector2D mercatorPos{static_cast<float>(mercatorPosCoord.x),
                                       static_cast<float>(mercatorPosCoord.y)};

            const auto worldPos = Vector2D::floor(mercatorToWorld(mercatorPos));
            const auto nearNode = m_model->getNodeAtPosition(worldPos, false, ACCURACY);
            if (nearNode) {
                const auto nearNodeIndex = m_model->getNodeIndex(nearNode);

                if (prevNodeIndex != INVALID_NODE && prevNodeIndex != nearNodeIndex) {
                    m_model->addEdgeFast(prevNodeIndex, nearNodeIndex, (int)0);
                    if (!currentMeta.m_isOneWay) {
                        m_model->addEdgeFast(nearNodeIndex, prevNodeIndex, (int)0);
                    }
                }

                prevNodeIndex = nearNodeIndex;
            } else {
                m_model->addNode(worldPos);
                const auto lastNodeIndex = m_model->getLastNodeIndex();

                if (prevNodeIndex != INVALID_NODE) {
                    m_model->addEdgeFast(prevNodeIndex, lastNodeIndex, (int)0);
                    if (!currentMeta.m_isOneWay) {
                        m_model->addEdgeFast(lastNodeIndex, prevNodeIndex, (int)0);
                    }
                }

                prevNodeIndex = lastNodeIndex;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        if (elapsed.count() >= 2) {
            lastUpdate = now;

            const auto currentPercentage =
                static_cast<float>(metaIndex + 1) / m_waysMeta.size() * 100.f;
            const auto elapsedSinceStart =
                std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();

            const auto remainingSeconds =
                elapsedSinceStart * (100.f - currentPercentage) / currentPercentage;

            const auto remMinutes = static_cast<int>(remainingSeconds) / 60;
            const auto remSeconds = static_cast<int>(remainingSeconds) % 60;

            std::cout << std::format(
                "Added {}/{} ways and {}/{} nodes... ({:.2f}%, ETA: {}m {}s)\n", metaIndex + 1,
                m_waysMeta.size(), m_model->getLastNodeIndex() + 1, m_parsedNodeCount,
                currentPercentage, remMinutes, remSeconds);
        }
    }

    m_model->sortEdges();

    std::cout << std::format(
        "Finished adding nodes. Added {}/{} ways and {}/{} nodes (ACCURACY = {}).\n",
        m_waysMeta.size(), m_waysMeta.size(), m_model->getLastNodeIndex() + 1, m_parsedNodeCount,
        ACCURACY);
}
#endif
