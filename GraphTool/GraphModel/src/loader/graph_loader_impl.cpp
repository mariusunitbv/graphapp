module;
#include <pch.h>

module graph_loader;

import graph_common;
import osm_loader;
import binary_loader;

void GraphLoader::loadOSM(GraphModel* model, const std::string_view osmFilePath) {
    common::ScopedTimer timer("GraphLoader::loadOSM({}, {})", (void*)model, osmFilePath);

    OSMLoader loader(model, osmFilePath);
    loader.loadGraph();
}

void GraphLoader::loadBinary(GraphModel* model, const std::string_view binaryFilePath) {
    common::ScopedTimer timer("GraphLoader::loadBinary({}, {})", (void*)model, binaryFilePath);

    BinaryLoader loader(model, binaryFilePath);
    loader.loadGraph();
}

void GraphLoader::saveBinary(GraphModel* model, const std::string_view binaryFilePath) {
    common::ScopedTimer timer("GraphLoader::saveBinary({}, {})", (void*)model, binaryFilePath);

    BinaryLoader loader(model, binaryFilePath);
    loader.saveGraph();
}
