module;
#include <pch.h>

export module graph_loader;

import graph_model;

export class GraphLoader {
   public:
    static void loadOSM(GraphModel* model, const std::string_view osmFilePath);
    static void loadBinary(GraphModel* model, const std::string_view binaryFilePath);

    static void saveBinary(GraphModel* model, const std::string_view binaryFilePath);
};
