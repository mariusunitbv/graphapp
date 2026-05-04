module;
#include <pch.h>

export module binary_loader;

import graph_model;
import heuristic;

export class BinaryLoader {
   public:
    BinaryLoader(GraphModel* model, const std::string_view binaryPath);

    void loadGraph();
    void loadGraphFromMemory(const char* data, size_t size);

    void saveGraph() const;

   private:
    enum class LoadState : uint8_t {
        READING_HEADER,
        READING_DATA,
        READING_METADATA_HEADER,
        READING_METADATA,
    };

    void processChunk(LZ4F_decompressionContext_t ctx, const char* input, size_t& inputPos,
                      size_t inputSize, std::vector<char>& pending, std::vector<char>& outBuffer,
                      uint32_t& nodeCount, uint32_t& processedNodeCount,
                      HeuristicType& heuristicType, uint32_t& metadataSize,
                      uint32_t& processedMetadataSize,
                      std::chrono::steady_clock::time_point& lastUpdate);

    GraphModel* m_model{nullptr};
    std::string m_binaryPath;
    LoadState m_loadState{LoadState::READING_HEADER};
};
