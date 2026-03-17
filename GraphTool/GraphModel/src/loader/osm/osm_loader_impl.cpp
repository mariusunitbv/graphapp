module;
#include <pch.h>

module osm_loader;

import graph_common;
import graph_model_defines;

#if defined(__EMSCRIPTEN__) || (defined(_WIN32) && !defined(_WIN64)) || defined(__i386__)
OSMLoader::OSMLoader(GraphModel*, const std::string_view) {
    common::Logger::get().error("OSM loading is not supported for current build.");
}

void OSMLoader::loadGraph() {
    common::Logger::get().error("OSM loading is not supported for current build.");
}
#else
static constexpr auto BOUND_LIMIT = 500'000;
static constexpr auto ACCURACY = 0.05f;

OSMLoader::OSMLoader(GraphModel* model, const std::string_view osmFile)
    : m_model(model), m_osmPath(osmFile), m_nodeForWaysNeeded(true) {
    if (osmFile.empty()) {
        GAPP_THROW("OSM file path cannot be empty");
    }

    m_mapBounds = {-BOUND_LIMIT, -BOUND_LIMIT, BOUND_LIMIT, BOUND_LIMIT};
}

void OSMLoader::loadGraph() {
    m_model->reserveArea(m_mapBounds);

    getNeededNodes();
    parseAndComputeBounds();
    addNodesToGraph();
}

Vector2D OSMLoader::mercatorToWorld(const Vector2D& mercatorPos) const {
    const auto nx = (mercatorPos.m_x - static_cast<float>(m_minX)) / m_dataWidth;
    const auto ny = (mercatorPos.m_y - static_cast<float>(m_minY)) / m_dataHeight;

    const auto x = -BOUND_LIMIT + m_scaledPaddingX + nx * m_scaledWidth;
    const auto y = -BOUND_LIMIT + m_scaledPaddingY + (1.f - ny) * m_scaledHeight;

    return {x, y};
}

void OSMLoader::getNeededNodes() {
    using namespace osmium;

    common::ScopedTimer timer("OSMLoader::getNeededNodes()");

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

            ++m_totalWayCount;
            for (const auto& node : nodes) {
                if (node.location().valid()) {
                    common::Logger::get().information(
                        "Node Locations for ways are already available in the file. No need to "
                        "parse "
                        "them separately.");
                    m_nodeForWaysNeeded = false;

                    return;
                }

                m_nodesLocations.emplace(node.ref(), osmium::Location{});
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        if (elapsed.count() >= 2) {
            lastUpdate = now;
            if (m_totalWayCount > 0) {
                common::Logger::get().information(
                    "Found {} ways with {} nodes that need to be parsed...", m_totalWayCount,
                    m_nodesLocations.size());
            }
        }
    }

    m_nodesForWays.reserve(m_nodesLocations.size());
    m_waysMeta.reserve(m_totalWayCount);

    common::Logger::get().information(
        "Finished checking nodes for ways. Found {} ways with {} nodes that need to be parsed.",
        m_totalWayCount, m_nodesLocations.size());
}

void OSMLoader::parseAndComputeBounds() {
    using namespace osmium;

    common::ScopedTimer timer("OSMLoader::parseAndComputeBounds()");

    uint32_t parsedNodeLocations = 0, parsedWayCount = 0;
    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();

    const auto readFlags =
        m_nodeForWaysNeeded ? osm_entity_bits::node | osm_entity_bits::way : osm_entity_bits::way;

    io::Reader reader(m_osmPath, readFlags, io::read_meta::no);
    while (auto buffer = reader.read()) {
        if (m_nodeForWaysNeeded && parsedNodeLocations != m_nodesLocations.size()) {
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
                common::Logger::get().information("Retrieved {}/{} nodes for ways...",
                                                  parsedNodeLocations, m_nodesLocations.size());
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

            ++parsedWayCount;

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
                common::Logger::get().information(
                    "Parsing {}/{} ways... ({:.2f}%)", parsedWayCount, m_totalWayCount,
                    static_cast<double>(parsedWayCount) / m_totalWayCount * 100.0);
            }
        }
    }

    common::Logger::get().information("Finished parsing. Parsed {} ways.", parsedWayCount);

    m_dataWidth = static_cast<float>(m_maxX - m_minX);
    m_dataHeight = static_cast<float>(m_maxY - m_minY);

    const auto canvasAspectRatio = m_mapBounds.width() / m_mapBounds.height();
    const auto dataAspectRatio = static_cast<float>((m_maxX - m_minX) / (m_maxY - m_minY));
    if (dataAspectRatio > canvasAspectRatio) {
        m_scaledWidth = m_mapBounds.width();
        m_scaledHeight = m_mapBounds.width() / dataAspectRatio;
        m_scaledPaddingY = (m_mapBounds.height() - m_scaledHeight) * 0.5f;
    } else {
        m_scaledWidth = m_mapBounds.height() * dataAspectRatio;
        m_scaledHeight = m_mapBounds.height();
        m_scaledPaddingX = (m_mapBounds.width() - m_scaledWidth) * 0.5f;
    }

    m_totalNodeCount = static_cast<uint32_t>(m_nodesLocations.size());
    m_nodesLocations =
        phmap::parallel_flat_hash_map<osmium::unsigned_object_id_type, osmium::Location>{};
}

void OSMLoader::addNodesToGraph() {
    size_t lastSampleWay = 0;

    common::ScopedTimer timer("OSMLoader::addNodesToGraph()");

    std::chrono::steady_clock::time_point lastSampleTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastUpdate = lastSampleTime;

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

            const auto worldPos = Vector2D::trunc(mercatorToWorld(mercatorPos));
            const auto nearNode = m_model->getNodeAtPosition(worldPos, ACCURACY);
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

            const auto processedWays = metaIndex + 1;

            const auto deltaWays = processedWays - lastSampleWay;
            const auto deltaSeconds =
                std::chrono::duration_cast<std::chrono::seconds>(now - lastSampleTime).count();

            double secondsPerWay = 0.0;
            if (deltaWays > 0) secondsPerWay = static_cast<double>(deltaSeconds) / deltaWays;

            lastSampleWay = processedWays;
            lastSampleTime = now;

            const auto remainingWays = m_waysMeta.size() - processedWays;
            const auto remainingSeconds = secondsPerWay * remainingWays;

            const auto remMinutes = static_cast<int>(remainingSeconds) / 60;
            const auto remSeconds = static_cast<int>(remainingSeconds) % 60;

            const auto currentPercentage =
                static_cast<double>(processedWays) / m_waysMeta.size() * 100.0;

            common::Logger::get().information(
                "Added {}/{} ways and {}/{} nodes... ({:.2f}%, ETA: {}m {}s)", processedWays,
                m_waysMeta.size(), m_model->getLastNodeIndex() + 1, m_totalNodeCount,
                currentPercentage, remMinutes, remSeconds);
        }
    }

    m_model->sortEdges();

    common::Logger::get().information(
        "Finished adding nodes. Added {}/{} ways and {}/{} nodes (ACCURACY = {}).",
        m_waysMeta.size(), m_waysMeta.size(), m_model->getLastNodeIndex() + 1, m_totalNodeCount,
        ACCURACY);
}
#endif
