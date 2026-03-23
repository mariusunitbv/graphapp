module;
#include <pch.h>

export module graph_loader;

import graph_model;
import osm_load_settings;

export class GraphLoader {
   public:
    static void loadOSM(GraphModel* model, const std::string_view osmFilePath,
                        const OSMLoadSettings& settings);
    static void loadBinary(GraphModel* model, const std::string_view binaryFilePath);
    static void loadBinaryFromMemory(GraphModel* model, const char* data, size_t size);

    static void saveBinary(GraphModel* model, const std::string_view binaryFilePath);
};
