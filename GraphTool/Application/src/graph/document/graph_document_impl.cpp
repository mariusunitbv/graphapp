module;
#include <pch.h>

module graph_document;

GraphDocument::GraphDocument(float displayWidth, float displayHeight) {
    m_viewModel.setModel(&m_model);
    m_viewModel.updateSceneSize(displayWidth, displayHeight);
}

GraphDocument::GraphDocument(GraphDocument&& rhs) noexcept
    : m_model(std::move(rhs.m_model)), m_viewModel(std::move(rhs.m_viewModel)) {
    m_viewModel.setModel(&m_model);
}

GraphDocument& GraphDocument::operator=(GraphDocument&& rhs) noexcept {
    if (this != &rhs) {
        m_model = std::move(rhs.m_model);
        m_viewModel = std::move(rhs.m_viewModel);

        m_viewModel.setModel(&m_model);
    }

    return *this;
}
