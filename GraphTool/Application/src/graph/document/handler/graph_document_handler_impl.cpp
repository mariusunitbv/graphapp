module;
#include <pch.h>

module graph_document_handler;

import graph_common;
import graph_loader;

void GraphDocumentHandler::initialize(GraphRenderer* graphRenderer) {
    m_graphRenderer = graphRenderer;
}

void GraphDocumentHandler::scheduleOpenDocument(const std::string& path) {
    m_documentToOpen = path;
}

void GraphDocumentHandler::scheduleCloseDocument(size_t index) { m_documentToClose = index; }

void GraphDocumentHandler::addEmptyDocument(std::vector<GraphDocument>& openDocuments) {
    openDocuments.emplace_back(0.f, 0.f);
    openDocuments.back().m_viewModel.addListener(m_graphRenderer);
}

void GraphDocumentHandler::processTasks(std::vector<GraphDocument>& openDocuments,
                                        size_t& currentOpenedDocument) {
    if (!m_documentToOpen.empty()) {
        if (!isDocumentAlreadyOpen(m_documentToOpen, openDocuments)) {
            try {
                GraphDocument newDoc(0, 0);
                GraphLoader::loadBinary(newDoc.m_viewModel.getModel(), m_documentToOpen);

                newDoc.m_viewModel.centerOnNode(0);

                openDocuments.push_back(std::move(newDoc));
                openDocuments.back().m_path = m_documentToOpen;
                openDocuments.back().m_viewModel.addListener(m_graphRenderer);
            } catch (const std::exception& e) {
                common::Logger::get().error("Error loading document '{}': {}",
                                            m_documentToOpen.c_str(), e.what());
            }
        } else {
            common::Logger::get().information("Document '{}' is already open, skipping.",
                                              m_documentToOpen);
        }
    }

    if (m_documentToClose != std::numeric_limits<size_t>::max()) {
        if (m_documentToClose < openDocuments.size()) {
            const auto openedDocumentSceneSize =
                openDocuments[m_documentToClose].m_viewModel.getSceneSize();

            openDocuments.erase(openDocuments.begin() + m_documentToClose);
            if (currentOpenedDocument >= m_documentToClose) {
                currentOpenedDocument = (currentOpenedDocument > 0) ? currentOpenedDocument - 1 : 0;
            }

            if (!openDocuments.empty() && m_documentToClose == currentOpenedDocument) {
                openDocuments[currentOpenedDocument].m_viewModel.updateSceneSize(
                    openedDocumentSceneSize.m_x, openedDocumentSceneSize.m_y);
            }
        } else {
            common::Logger::get().warning("Invalid document index to close: {}", m_documentToClose);
        }
    }

    m_documentToOpen.clear();
    m_documentToClose = std::numeric_limits<size_t>::max();
}

bool GraphDocumentHandler::isDocumentAlreadyOpen(
    const std::string& path, const std::vector<GraphDocument>& openDocuments) const {
    for (const auto& doc : openDocuments) {
        if (doc.m_path == path) {
            return true;
        }
    }

    return false;
}
