module;
#include <pch.h>

export module graph_document_handler;

export import graph_document;

import graph_renderer;

export class GraphDocumentHandler {
   public:
    void initialize(GraphRenderer* graphRenderer);

    void scheduleOpenDocument(const std::string& path);
    void scheduleCloseDocument(size_t index);

    void addEmptyDocument(std::vector<GraphDocument>& openDocuments);

    void processTasks(std::vector<GraphDocument>& openDocuments, size_t& currentOpenedDocument);

   private:
    bool isDocumentAlreadyOpen(const std::string& path,
                               const std::vector<GraphDocument>& openDocuments) const;

    GraphRenderer* m_graphRenderer{nullptr};

    std::string m_documentToOpen;
    size_t m_documentToClose{std::numeric_limits<size_t>::max()};
};
