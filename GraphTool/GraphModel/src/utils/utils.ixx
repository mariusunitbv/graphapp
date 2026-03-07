module;
#include <pch.h>

export module graph_utils;

import graph_model;

export class Utils {
   public:
    static void loadOSM(GraphModel* model, const std::string_view osmFilePath);

    static void loadJSON(GraphModel* model, const std::string_view jsonFilePath);
    static void saveJSON(GraphModel* model, const std::string_view jsonFilePath);
};
