module;
#include <pch.h>

module graph_loader;

import osm_loader;
import json_loader;

void GraphLoader::loadOSM(GraphModel* model, const std::string_view osmFilePath) {
    OSMLoader loader(model, osmFilePath);

    const auto now = std::chrono::steady_clock::now();
    loader.loadGraph();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - now)
                              .count();

    std::cout << "Loaded \"" << osmFilePath << "\" file in " << duration << " ms.\n";
}

void GraphLoader::loadJSON(GraphModel* model, const std::string_view jsonFilePath) {
    JsonLoader loader(model, jsonFilePath);
    loader.loadGraph();
}

void GraphLoader::saveJSON(GraphModel* model, const std::string_view jsonFilePath) {
    JsonLoader loader(model, jsonFilePath);
    loader.saveGraph();
}
