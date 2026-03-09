module;
#include <pch.h>
#include <simdjson.h>

module json_loader;

JsonLoader::JsonLoader(GraphModel* model, const std::string_view jsonPath)
    : m_model(model), m_jsonPath(jsonPath) {
    if (jsonPath.empty()) {
        GAPP_THROW("JSON path cannot be empty");
    }
}

void JsonLoader::loadGraphToJson() const {
    using namespace simdjson;

    ondemand::parser parser;
    const auto json = padded_string::load(m_jsonPath);
    auto doc = parser.iterate(json);

    m_model->beginBulkInsert();

    const auto minX = static_cast<float>(doc["minX"].get_double().value());
    const auto minY = static_cast<float>(doc["minY"].get_double().value());
    const auto maxX = static_cast<float>(doc["maxX"].get_double().value());
    const auto maxY = static_cast<float>(doc["maxY"].get_double().value());

    const auto nodeCount = doc["nodeCount"].get_uint64().value();
    m_model->reserveNodes(nodeCount);
    m_model->reserveArea({minX, minY, maxX, maxY});

    for (ondemand::array node : doc["nodes"]) {
        float x = std::numeric_limits<float>::max(), y = std::numeric_limits<float>::max();
        int degree = -1;

        for (ondemand::value nodeData : node) {
            if (x == std::numeric_limits<float>::max()) {
                x = static_cast<float>(nodeData.get_double().value());
            } else if (y == std::numeric_limits<float>::max()) {
                y = static_cast<float>(nodeData.get_double().value());
                m_model->addNode({x, y});
            } else if (degree == -1) {
                degree = static_cast<int>(nodeData.get_int64().value());
            } else {
                auto neighbours = nodeData.get_array();
                for (ondemand::array neighbour : neighbours) {
                    NodeIndex_t dest = INVALID_NODE;
                    std::optional<int> weight;

                    for (ondemand::value neighbourData : neighbour) {
                        if (dest == INVALID_NODE) {
                            dest = static_cast<NodeIndex_t>(neighbourData.get_uint64().value());
                        } else {
                            weight = static_cast<int>(neighbourData.get_int64().value());
                        }
                    }

                    m_model->addEdge(m_model->getLastNodeIndex(), dest, weight.value_or(0));
                }
            }
        }
    }

    m_model->endBulkInsert();
}

void JsonLoader::saveGraphToJson() const {
    using namespace simdjson;

    builder::string_builder sb;
    sb.start_object();
    {
        const auto [min, max] = m_model->getGraphBounds();
        const auto nodeCount = m_model->getLastNodeIndex() + 1;

        sb.append_key_value<"minX">(min.m_x);
        sb.append_comma();
        sb.append_key_value<"minY">(min.m_y);
        sb.append_comma();
        sb.append_key_value<"maxX">(max.m_x);
        sb.append_comma();
        sb.append_key_value<"maxY">(max.m_y);
        sb.append_comma();

        sb.append_key_value<"nodeCount">(nodeCount);
        sb.append_comma();

        sb.escape_and_append_with_quotes("nodes");
        sb.append_colon();
        sb.start_array();
        {
            for (NodeIndex_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
                const auto node = m_model->getNode(nodeIndex);
                const auto pos = node->getWorldPos();

                sb.start_array();
                {
                    sb.append(static_cast<int>(pos.m_x));
                    sb.append_comma();
                    sb.append(static_cast<int>(pos.m_y));

                    const auto degree = m_model->getNodeDegree(nodeIndex);
                    if (degree != 0) {
                        sb.append_comma();
                        sb.append(degree);
                        sb.append_comma();

                        sb.start_array();
                        {
                            struct EdgeVisitorData {
                                builder::string_builder* sb;
                                bool first;
                            } edgeVisitorData{&sb, true};
                            m_model->visitNeighbours(
                                nodeIndex, &edgeVisitorData,
                                [](void* userData, NodeIndex_t dest, int weight) {
                                    auto* data = static_cast<EdgeVisitorData*>(userData);

                                    const auto sb = data->sb;
                                    if (!data->first) {
                                        sb->append_comma();
                                    }

                                    data->first = false;

                                    sb->start_array();
                                    {
                                        sb->append(dest);
                                        if (weight != 0) {
                                            sb->append_comma();
                                            sb->append(weight);
                                        }
                                    }
                                    sb->end_array();

                                    return true;
                                });
                        }
                        sb.end_array();
                    }
                }
                sb.end_array();

                if (nodeIndex < nodeCount - 1) {
                    sb.append_comma();
                }
            }
        }
        sb.end_array();
    }
    sb.end_object();

    std::ofstream file(m_jsonPath);
    file << sb.view();
}
