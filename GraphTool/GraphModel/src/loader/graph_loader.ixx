module;
#include <pch.h>

export module graph_loader;

import graph_model;

export class GraphLoader {
   public:
    static void loadOSM(GraphModel* model, const std::string_view osmFilePath);
    static void loadJSON(GraphModel* model, const std::string_view jsonFilePath);

    static void saveJSON(GraphModel* model, const std::string_view jsonFilePath);
};
