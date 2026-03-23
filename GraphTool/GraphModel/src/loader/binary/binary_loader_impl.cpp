module;
#include <pch.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

module binary_loader;

import graph_common;

static constexpr size_t INPUT_BLOCK_SIZE = 4llu << 20;
static constexpr size_t OUTPUT_BLOCK_SIZE = 16llu << 20;

template <typename T>
static T readValue(const char*& ptr) {
    T value;
    std::memcpy(&value, ptr, sizeof(T));
    ptr += sizeof(T);
    return value;
}

template <typename T>
static void writeValue(std::vector<char>& buffer, const T& value) {
    const char* ptr = reinterpret_cast<const char*>(&value);
    buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
}

#ifdef __EMSCRIPTEN__
EM_JS(void, downloadFile, (const char* data, int size), {
    var blob = new Blob([Module.HEAPU8.subarray(data, data + size)], {
        type:
            'application/octet-stream'
    });
    var link = document.createElement('a');
    link.href = URL.createObjectURL(blob);
    link.download = "graph.bin";
    link.click();
});

static void flushToLZ4(std::vector<char>& downloadBuffer, LZ4F_compressionContext_t& ctx,
                       std::vector<char>& outBuffer, std::vector<char>& rawBuffer) {
    const auto compressedSize = LZ4F_compressUpdate(ctx, outBuffer.data(), outBuffer.size(),
                                                    rawBuffer.data(), rawBuffer.size(), nullptr);
    if (LZ4F_isError(compressedSize)) {
        LZ4F_freeCompressionContext(ctx);
        GAPP_THROW("LZ4 compression error: " + std::string(LZ4F_getErrorName(compressedSize)));
    }

    if (compressedSize > 0) {
        downloadBuffer.insert(downloadBuffer.end(), outBuffer.data(),
                              outBuffer.data() + compressedSize);
    }

    rawBuffer.clear();
}
#else
static void flushToLZ4(std::ofstream& file, LZ4F_compressionContext_t& ctx,
                       std::vector<char>& outBuffer, std::vector<char>& rawBuffer) {
    const auto compressedSize = LZ4F_compressUpdate(ctx, outBuffer.data(), outBuffer.size(),
                                                    rawBuffer.data(), rawBuffer.size(), nullptr);
    if (LZ4F_isError(compressedSize)) {
        LZ4F_freeCompressionContext(ctx);
        GAPP_THROW("LZ4 compression error: " + std::string(LZ4F_getErrorName(compressedSize)));
    }

    if (compressedSize > 0) {
        file.write(outBuffer.data(), compressedSize);
    }

    rawBuffer.clear();
}
#endif

BinaryLoader::BinaryLoader(GraphModel* model, const std::string_view binaryPath)
    : m_model(model), m_binaryPath(binaryPath) {
    if (binaryPath.empty()) {
        GAPP_THROW("Binary path cannot be empty");
    }
}

void BinaryLoader::loadGraph() const {
    std::ifstream file(m_binaryPath, std::ios::binary);
    if (!file) {
        common::Logger::get().error("Failed to open LZ4 file: {}", m_binaryPath);
        return;
    }

    LZ4F_decompressionContext_t ctx;
    if (LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION) != 0) {
        GAPP_THROW("Failed to create LZ4 decompress context");
    }

    std::vector<char> inBuffer(INPUT_BLOCK_SIZE);
    std::vector<char> outBuffer(OUTPUT_BLOCK_SIZE);

    size_t inputPos = 0, inputSize = 0;
    bool headerRead = false;

    std::vector<char> pending;
    uint32_t nodeCount = 0, processedNodeCount = 0;

    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();

    m_model->beginBulkInsert();
    while (true) {
        if (inputPos >= inputSize) {
            file.read(inBuffer.data(), INPUT_BLOCK_SIZE);
            inputSize = static_cast<size_t>(file.gcount());
            if (inputSize == 0) {
                break;
            }

            inputPos = 0;
        }

        processChunk(ctx, inBuffer.data(), inputPos, inputSize, pending, outBuffer, headerRead,
                     nodeCount, processedNodeCount, lastUpdate);
    }

    m_model->endBulkInsert();
    LZ4F_freeDecompressionContext(ctx);
}

void BinaryLoader::loadGraphFromMemory(const char* data, size_t size) const {
    LZ4F_decompressionContext_t ctx;
    if (LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION) != 0) {
        GAPP_THROW("Failed to create LZ4 decompress context");
    }

    std::vector<char> outBuffer(OUTPUT_BLOCK_SIZE);
    size_t inputPos = 0;
    bool headerRead = false;

    std::vector<char> pending;
    uint32_t nodeCount = 0, processedNodeCount = 0;

    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();

    m_model->beginBulkInsert();

    processChunk(ctx, data, inputPos, size, pending, outBuffer, headerRead, nodeCount,
                 processedNodeCount, lastUpdate);

    m_model->endBulkInsert();
    LZ4F_freeDecompressionContext(ctx);
}

void BinaryLoader::saveGraph() const {
    const auto [min, max] = m_model->getGraphBounds();
    const auto nodeCount = m_model->getNodeCount();
    if (nodeCount == 0) {
        common::Logger::get().warning("Graph is empty, skipping save.");
        return;
    }

#ifdef __EMSCRIPTEN__
    std::vector<char> buffer;
    buffer.reserve(OUTPUT_BLOCK_SIZE);
#else
    const auto parentDir = std::filesystem::path(m_binaryPath).parent_path();
    if (!parentDir.empty()) {
        common::FileSystem::get().createFolder(parentDir.string());
    }

    std::ofstream file(m_binaryPath, std::ios::binary);
    if (!file) {
        GAPP_THROW("Failed to open binary file for writing: " + m_binaryPath);
    }
#endif

    LZ4F_compressionContext_t ctx;
    if (LZ4F_createCompressionContext(&ctx, LZ4F_VERSION) != 0) {
        GAPP_THROW("Failed to create LZ4 decompress context");
    }

    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();

    std::vector<char> rawBuffer;
    std::vector<char> outBuffer(LZ4F_compressBound(OUTPUT_BLOCK_SIZE, nullptr));
    rawBuffer.reserve(INPUT_BLOCK_SIZE);

    auto headerSize = LZ4F_compressBegin(ctx, outBuffer.data(), outBuffer.size(), nullptr);

#ifdef __EMSCRIPTEN__
    buffer.insert(buffer.end(), outBuffer.data(), outBuffer.data() + headerSize);
#else
    file.write(outBuffer.data(), headerSize);
#endif

    writeValue(rawBuffer, min.m_x);
    writeValue(rawBuffer, min.m_y);
    writeValue(rawBuffer, max.m_x);
    writeValue(rawBuffer, max.m_y);
    writeValue(rawBuffer, nodeCount);

    for (NodeIndex_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        const auto [x, y] = m_model->getNode(nodeIndex)->getWorldPos();
        const auto degree = m_model->getNodeDegree(nodeIndex);

        writeValue(rawBuffer, x);
        writeValue(rawBuffer, y);
        writeValue(rawBuffer, degree);

        m_model->visitNeighbours(nodeIndex, &rawBuffer,
                                 [](void* userData, NodeIndex_t dest, int weight) {
                                     auto& rawBuffer = *static_cast<std::vector<char>*>(userData);

                                     writeValue(rawBuffer, dest);
                                     writeValue(rawBuffer, weight);

                                     return true;
                                 });

        if (rawBuffer.size() >= INPUT_BLOCK_SIZE) {
#ifdef __EMSCRIPTEN__
            flushToLZ4(buffer, ctx, outBuffer, rawBuffer);
#else
            flushToLZ4(file, ctx, outBuffer, rawBuffer);
#endif
        }

        const auto now = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        if (duration.count() >= 2) {
            common::Logger::get().information("Saved {}/{} nodes... ({:.2f}%)", nodeIndex,
                                              nodeCount,
                                              static_cast<double>(nodeIndex) / nodeCount * 100.0);
            lastUpdate = now;
        }
    }

    if (!rawBuffer.empty()) {
#ifdef __EMSCRIPTEN__
        flushToLZ4(buffer, ctx, outBuffer, rawBuffer);
#else
        flushToLZ4(file, ctx, outBuffer, rawBuffer);
#endif
    }

    const auto endResult = LZ4F_compressEnd(ctx, outBuffer.data(), outBuffer.size(), nullptr);
    if (LZ4F_isError(endResult)) {
        LZ4F_freeCompressionContext(ctx);
        GAPP_THROW("LZ4 compression error: " + std::string(LZ4F_getErrorName(endResult)));
    }

#ifdef __EMSCRIPTEN__
    buffer.insert(buffer.end(), outBuffer.data(), outBuffer.data() + endResult);
#else
    file.write(outBuffer.data(), endResult);
#endif

    LZ4F_freeCompressionContext(ctx);

#ifdef __EMSCRIPTEN__
    downloadFile(buffer.data(), static_cast<int>(buffer.size()));
#endif
}

void BinaryLoader::processChunk(LZ4F_decompressionContext_t ctx, const char* input,
                                size_t& inputPos, size_t inputSize, std::vector<char>& pending,
                                std::vector<char>& outBuffer, bool& headerRead, uint32_t& nodeCount,
                                uint32_t& processedNodeCount,
                                std::chrono::steady_clock::time_point& lastUpdate) const {
    while (inputPos < inputSize) {
        size_t inSize = inputSize - inputPos;
        size_t outSize = OUTPUT_BLOCK_SIZE;

        const auto result =
            LZ4F_decompress(ctx, outBuffer.data(), &outSize, input + inputPos, &inSize, nullptr);

        if (LZ4F_isError(result)) {
            LZ4F_freeDecompressionContext(ctx);
            GAPP_THROW("LZ4 decompression error: " + std::string(LZ4F_getErrorName(result)));
        }

        if (outSize > 0) {
            pending.insert(pending.end(), outBuffer.data(), outBuffer.data() + outSize);

            const char* ptr = pending.data();
            size_t available = pending.size();
            size_t consumed = 0;

            if (!headerRead) {
                constexpr size_t headerSize = sizeof(float) * 4 + sizeof(uint32_t);
                if (available < headerSize) {
                    inputPos += inSize;
                    continue;
                }

                const auto minX = readValue<float>(ptr);
                const auto minY = readValue<float>(ptr);
                const auto maxX = readValue<float>(ptr);
                const auto maxY = readValue<float>(ptr);
                nodeCount = readValue<uint32_t>(ptr);

                m_model->reserveNodes(nodeCount);
                m_model->reserveArea({minX, minY, maxX, maxY});

                headerRead = true;
                available -= headerSize;
                consumed += headerSize;
            }

            if (headerRead) {
                constexpr size_t nodeSize = sizeof(float) * 2 + sizeof(uint32_t);
                constexpr size_t edgeSize = sizeof(NodeIndex_t) + sizeof(int);

                while (processedNodeCount < nodeCount && available >= nodeSize) {
                    const char* tmpPtr = ptr;

                    const auto x = readValue<float>(tmpPtr);
                    const auto y = readValue<float>(tmpPtr);
                    const auto degree = readValue<uint32_t>(tmpPtr);

                    const auto totalSize = nodeSize + degree * edgeSize;
                    if (available < totalSize) break;

                    m_model->addNode({x, y});
                    const auto nodeIndex = m_model->getLastNodeIndex();
                    m_model->reserveDegree(nodeIndex, degree);

                    for (uint32_t j = 0; j < degree; ++j) {
                        const auto dest = readValue<NodeIndex_t>(tmpPtr);
                        const auto weight = readValue<int>(tmpPtr);
                        m_model->addEdge(nodeIndex, dest, weight);
                    }

                    ptr = tmpPtr;
                    available -= totalSize;
                    consumed += totalSize;
                    ++processedNodeCount;

                    const auto now = std::chrono::steady_clock::now();
                    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate)
                            .count() >= 2) {
                        common::Logger::get().information(
                            "Loaded {}/{} nodes... ({:.2f}%)", processedNodeCount, nodeCount,
                            static_cast<double>(processedNodeCount) / nodeCount * 100.0);
                        lastUpdate = now;
                    }
                }
            }

            if (consumed > 0) {
                pending.erase(pending.begin(), pending.begin() + consumed);
            }
        }

        inputPos += inSize;
        if (result == 0) {
            break;
        }
    }
}
