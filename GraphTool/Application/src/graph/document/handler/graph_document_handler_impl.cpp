module;
#include <pch.h>

module graph_document_handler;

import graph_common;
import graph_loader;

void GraphDocumentHandler::addListener(IGraphDocumentListener* listener) {
    m_listeners.push_back(listener);
}

void GraphDocumentHandler::scheduleOpenDocument(const char* data, size_t size) {
    std::unique_lock lock(m_documentsMutex);
    m_binaryDocumentsToOpen.emplace_back(std::string(data, size));
}

void GraphDocumentHandler::scheduleOpenDocument(const std::string& path) {
    std::unique_lock lock(m_documentsMutex);
    m_documentsToOpen.push_back(path);
}

void GraphDocumentHandler::scheduleOpenedDocument(size_t index) { m_documentToOpen = index; }

void GraphDocumentHandler::scheduleCloseDocument(size_t index) { m_documentToClose = index; }

bool GraphDocumentHandler::canDirectlySaveCurrentDocument() const {
    std::shared_lock lock(m_documentsMutex);

    const auto& currentDoc = m_openedDocuments[m_currentOpenedDocument];
    return !currentDoc.m_path.empty();
}

void GraphDocumentHandler::saveCurrentDocument(const std::string& path) {
    auto& currentDoc = getCurrentOpenedDocument();

    GraphLoader::saveBinary(currentDoc.m_viewModel.getModel(), path);
    currentDoc.m_path = path;
}

void GraphDocumentHandler::addEmptyDocument() {
    cancelRunningUpdates();
    m_openedDocuments.emplace_back(0.f, 0.f);

    for (auto* listener : m_listeners) {
        listener->onDocumentAdded(m_openedDocuments.back());
    }

    setCurrentDocument(m_openedDocuments.size() - 1);
}

void GraphDocumentHandler::processTasks() {
    std::unique_lock lock(m_documentsMutex);
    for (const auto& documentToOpen : m_documentsToOpen) {
        if (documentToOpen.empty()) {
            addEmptyDocument();
            continue;
        }

        const auto openedDocumentIndexOpt = getDocumentOpenedIndex(documentToOpen);
        if (!openedDocumentIndexOpt) {
            try {
                GraphDocument newDoc(0, 0);

                const auto isOSM = isOSMFile(documentToOpen);
                if (isOSM) {
                    GraphLoader::loadOSM(newDoc.m_viewModel.getModel(), documentToOpen,
                                         m_osmLoadSettings);
                } else {
                    GraphLoader::loadBinary(newDoc.m_viewModel.getModel(), documentToOpen);
                }

                newDoc.m_viewModel.centerOnNode(0);

                cancelRunningUpdates();
                m_openedDocuments.push_back(std::move(newDoc));

                if (!isOSM) {
                    m_openedDocuments.back().m_path = documentToOpen;
                }

                for (auto* listener : m_listeners) {
                    listener->onDocumentAdded(m_openedDocuments.back());
                }

                setCurrentDocument(m_openedDocuments.size() - 1);
            } catch (const std::exception& e) {
                common::Logger::get().error("Error loading document '{}': {}", documentToOpen,
                                            e.what());
            }
        } else {
            setCurrentDocument(openedDocumentIndexOpt.value());
            common::Logger::get().information("Document '{}' is already open, skipping.",
                                              documentToOpen);
        }
    }

    for (const auto& binaryData : m_binaryDocumentsToOpen) {
        try {
            GraphDocument newDoc(0, 0);

            common::Logger::get().information("Loading document from memory (size: {} bytes)...",
                                              binaryData.size());

            GraphLoader::loadBinaryFromMemory(newDoc.m_viewModel.getModel(), binaryData.data(),
                                              binaryData.size());

            common::Logger::get().information("Document loaded from memory successfully.");

            newDoc.m_viewModel.centerOnNode(0);

            cancelRunningUpdates();
            m_openedDocuments.push_back(std::move(newDoc));

            for (auto* listener : m_listeners) {
                listener->onDocumentAdded(m_openedDocuments.back());
            }

            setCurrentDocument(m_openedDocuments.size() - 1);
        } catch (const std::exception& e) {
            common::Logger::get().error("Error loading document from memory: {}", e.what());
        }
    }

    if (m_documentToClose != std::numeric_limits<size_t>::max()) {
        cancelRunningUpdates();

        if (m_documentToClose < m_openedDocuments.size()) {
            m_openedDocuments.erase(m_openedDocuments.begin() + m_documentToClose);

            if (!m_openedDocuments.empty() && m_currentOpenedDocument >= m_documentToClose) {
                setCurrentDocument(m_currentOpenedDocument > 0 ? m_currentOpenedDocument - 1 : 0);
            }
        } else {
            common::Logger::get().warning("Invalid document index to close: {}", m_documentToClose);
        }
    }

    if (m_documentToOpen != std::numeric_limits<size_t>::max()) {
        cancelRunningUpdates();

        if (m_documentToOpen < m_openedDocuments.size()) {
            setCurrentDocument(m_documentToOpen);
        } else {
            common::Logger::get().warning("Invalid document index to open: {}", m_documentToOpen);
        }
    }

    m_documentsToOpen.clear();
    m_binaryDocumentsToOpen.clear();
    m_documentToOpen = std::numeric_limits<size_t>::max();
    m_documentToClose = std::numeric_limits<size_t>::max();
}

bool GraphDocumentHandler::isAnyDocumentOpen() const { return !m_openedDocuments.empty(); }

GraphDocument& GraphDocumentHandler::getCurrentOpenedDocument() {
    std::shared_lock lock(m_documentsMutex);
    return m_openedDocuments[m_currentOpenedDocument];
}

size_t GraphDocumentHandler::getCurrentOpenedDocumentIndex() const {
    std::shared_lock lock(m_documentsMutex);
    return m_currentOpenedDocument;
}

const std::vector<GraphDocument>& GraphDocumentHandler::getOpenedDocuments() const {
    std::shared_lock lock(m_documentsMutex);
    return m_openedDocuments;
}

OSMLoadSettings& GraphDocumentHandler::getOSMLoadSettings() { return m_osmLoadSettings; }

void GraphDocumentHandler::cancelRunningUpdates() {
    for (auto& doc : m_openedDocuments) {
        doc.m_viewModel.cancelRunningUpdate();
    }
}

std::optional<size_t> GraphDocumentHandler::getDocumentOpenedIndex(const std::string& path) const {
    for (size_t i = 0; i < m_openedDocuments.size(); ++i) {
        const auto& doc = m_openedDocuments[i];
        if (doc.m_path == path) {
            return i;
        }
    }

    return std::nullopt;
}

bool GraphDocumentHandler::isOSMFile(const std::string& path) const {
    return path.ends_with(".osm") || path.ends_with(".pbf");
}

void GraphDocumentHandler::setCurrentDocument(size_t documentToOpenIndex) {
    m_currentOpenedDocument = documentToOpenIndex;
    for (auto* listener : m_listeners) {
        listener->onDocumentChanged(m_openedDocuments[m_currentOpenedDocument]);
    }
}
