module;
#include <pch.h>

export module graph_document_handler;

export import graph_document;

import graph_renderer;
import graph_document_listener;

export class GraphDocumentHandler {
   public:
    void initialize(GraphRenderer* graphRenderer);

    void addListener(IGraphDocumentListener* listener);

    void scheduleOpenDocument(const std::string& path);
    void scheduleSetOpenedDocument(size_t index);
    void scheduleCloseDocument(size_t index);

    void addEmptyDocument(std::vector<GraphDocument>& openDocuments);

    void processTasks(std::vector<GraphDocument>& openDocuments, size_t& currentOpenedDocument);

   private:
    bool isDocumentAlreadyOpen(const std::string& path,
                               const std::vector<GraphDocument>& openDocuments,
                               size_t& documentIndex) const;

    void setCurrentDocument(std::vector<GraphDocument>& openDocuments,
                            size_t& currentOpenedDocument, size_t documentToOpenIndex);

    GraphRenderer* m_graphRenderer{nullptr};

    std::vector<IGraphDocumentListener*> m_listeners{};

    std::vector<std::string> m_documentsToOpen;
    size_t m_documentToOpen{std::numeric_limits<size_t>::max()};
    size_t m_documentToClose{std::numeric_limits<size_t>::max()};
};
