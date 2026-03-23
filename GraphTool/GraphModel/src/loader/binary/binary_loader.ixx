module;
#include <pch.h>

export module binary_loader;

import graph_model;

export class BinaryLoader {
   public:
    BinaryLoader(GraphModel* model, const std::string_view binaryPath);

    void loadGraph() const;
    void loadGraphFromMemory(const char* data, size_t size) const;

    void saveGraph() const;

   private:
    void processChunk(LZ4F_decompressionContext_t ctx, const char* input, size_t& inputPos,
                      size_t inputSize, std::vector<char>& pending, std::vector<char>& outBuffer,
                      bool& headerRead, uint32_t& nodeCount, uint32_t& processedNodeCount,
                      std::chrono::steady_clock::time_point& lastUpdate) const;

    GraphModel* m_model{nullptr};
    std::string m_binaryPath;
};
