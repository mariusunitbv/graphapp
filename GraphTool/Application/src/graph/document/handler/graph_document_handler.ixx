module;
#include <pch.h>

export module graph_document_handler;

export import graph_document;

import graph_renderer;
import graph_document_listener;

import osm_load_settings;

export class GraphDocumentHandler {
   public:
    void addListener(IGraphDocumentListener* listener);

    void scheduleOpenDocument(const char* data, size_t size);
    void scheduleOpenDocument(const std::string& path);
    void scheduleOpenedDocument(size_t index);
    void scheduleCloseDocument(size_t index);

    bool canDirectlySaveCurrentDocument() const;
    void saveCurrentDocument(const std::string& path);

    void addEmptyDocument();

    void processTasks();

    bool isAnyDocumentOpen() const;

    GraphDocument& getCurrentOpenedDocument();
    size_t getCurrentOpenedDocumentIndex() const;

    const std::vector<GraphDocument>& getOpenedDocuments() const;
    OSMLoadSettings& getOSMLoadSettings();

   private:
    std::optional<size_t> getDocumentOpenedIndex(const std::string& path) const;

    bool isOSMFile(const std::string& path) const;

    void cancelRunningUpdates();
    void setCurrentDocument(size_t documentToOpenIndex);

    std::vector<IGraphDocumentListener*> m_listeners{};

    mutable std::shared_mutex m_documentsMutex{};

    std::vector<std::string> m_binaryDocumentsToOpen{};
    std::vector<std::string> m_documentsToOpen{};
    size_t m_documentToOpen{std::numeric_limits<size_t>::max()};
    size_t m_documentToClose{std::numeric_limits<size_t>::max()};

    std::vector<GraphDocument> m_openedDocuments{};
    size_t m_currentOpenedDocument{0};

    OSMLoadSettings m_osmLoadSettings{};
};
