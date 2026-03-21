module;
#include <pch.h>

module graph_document_handler;

import graph_common;
import graph_loader;

void GraphDocumentHandler::initialize(GraphRenderer* graphRenderer) {
    m_graphRenderer = graphRenderer;
}

void GraphDocumentHandler::addListener(IGraphDocumentListener* listener) {
    m_listeners.push_back(listener);
}

void GraphDocumentHandler::scheduleOpenDocument(const std::string& path) {
    m_documentsToOpen.push_back(path);
}

void GraphDocumentHandler::scheduleSetOpenedDocument(size_t index) { m_documentToOpen = index; }

void GraphDocumentHandler::scheduleCloseDocument(size_t index) { m_documentToClose = index; }

void GraphDocumentHandler::addEmptyDocument(std::vector<GraphDocument>& openDocuments) {
    openDocuments.emplace_back(0.f, 0.f);
    openDocuments.back().m_viewModel.addListener(m_graphRenderer);

    setCurrentDocument(openDocuments, m_documentToOpen, openDocuments.size() - 1);
}

void GraphDocumentHandler::processTasks(std::vector<GraphDocument>& openDocuments,
                                        size_t& currentOpenedDocument) {
    for (size_t i = 0; i < m_documentsToOpen.size(); ++i) {
        const auto& documentToOpen = m_documentsToOpen[i];
        if (documentToOpen.empty()) {
            continue;
        }

        size_t openedDocumentIndex;
        if (!isDocumentAlreadyOpen(documentToOpen, openDocuments, openedDocumentIndex)) {
            for (auto& doc : openDocuments) {
                doc.m_viewModel.cancelRunningUpdate();
            }

            try {
                GraphDocument newDoc(0, 0);
                GraphLoader::loadBinary(newDoc.m_viewModel.getModel(), documentToOpen);

                newDoc.m_viewModel.centerOnNode(0);

                openDocuments.push_back(std::move(newDoc));
                openDocuments.back().m_path = documentToOpen;
                openDocuments.back().m_viewModel.addListener(m_graphRenderer);

                setCurrentDocument(openDocuments, currentOpenedDocument, openDocuments.size() - 1);
            } catch (const std::exception& e) {
                common::Logger::get().error("Error loading document '{}': {}", documentToOpen,
                                            e.what());
            }
        } else {
            setCurrentDocument(openDocuments, currentOpenedDocument, openedDocumentIndex);
            common::Logger::get().information("Document '{}' is already open, skipping.",
                                              documentToOpen);
        }
    }

    if (m_documentToClose != std::numeric_limits<size_t>::max()) {
        for (auto& doc : openDocuments) {
            doc.m_viewModel.cancelRunningUpdate();
        }

        if (m_documentToClose < openDocuments.size()) {
            openDocuments.erase(openDocuments.begin() + m_documentToClose);
            if (!openDocuments.empty() && currentOpenedDocument >= m_documentToClose) {
                const auto newCurrentDocument =
                    (currentOpenedDocument > 0) ? currentOpenedDocument - 1 : 0;
                setCurrentDocument(openDocuments, currentOpenedDocument, newCurrentDocument);
            }
        } else {
            common::Logger::get().warning("Invalid document index to close: {}", m_documentToClose);
        }
    }

    if (m_documentToOpen != std::numeric_limits<size_t>::max()) {
        for (auto& doc : openDocuments) {
            doc.m_viewModel.cancelRunningUpdate();
        }

        if (m_documentToOpen < openDocuments.size()) {
            setCurrentDocument(openDocuments, currentOpenedDocument, m_documentToOpen);
        } else {
            common::Logger::get().warning("Invalid document index to open: {}", m_documentToOpen);
        }
    }

    m_documentsToOpen.clear();
    m_documentToOpen = std::numeric_limits<size_t>::max();
    m_documentToClose = std::numeric_limits<size_t>::max();
}

bool GraphDocumentHandler::isDocumentAlreadyOpen(const std::string& path,
                                                 const std::vector<GraphDocument>& openDocuments,
                                                 size_t& documentIndex) const {
    for (size_t i = 0; i < openDocuments.size(); ++i) {
        const auto& doc = openDocuments[i];
        if (doc.m_path == path) {
            documentIndex = i;
            return true;
        }
    }

    documentIndex = std::numeric_limits<size_t>::max();
    return false;
}

void GraphDocumentHandler::setCurrentDocument(std::vector<GraphDocument>& openDocuments,
                                              size_t& currentOpenedDocument,
                                              size_t documentToOpenIndex) {
    currentOpenedDocument = documentToOpenIndex;
    for (auto* listener : m_listeners) {
        listener->onDocumentChanged(openDocuments[currentOpenedDocument]);
    }
}
