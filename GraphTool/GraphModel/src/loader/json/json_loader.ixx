module;
#include <pch.h>

export module json_loader;

import graph_model;

export class JsonLoader {
   public:
    JsonLoader(GraphModel* model, const std::string_view jsonPath);

    void loadGraph() const;
    void saveGraph() const;

   private:
    GraphModel* m_model{nullptr};
    std::string m_jsonPath;
};
