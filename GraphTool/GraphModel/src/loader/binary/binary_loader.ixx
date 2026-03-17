module;
#include <pch.h>

export module binary_loader;

import graph_model;

export class BinaryLoader {
   public:
    BinaryLoader(GraphModel* model, const std::string_view binaryPath);

    void loadGraph() const;
    void saveGraph() const;

   private:
    GraphModel* m_model{nullptr};
    std::string m_binaryPath;
};
