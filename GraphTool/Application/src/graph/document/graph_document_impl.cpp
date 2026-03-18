module;
#include <pch.h>

module graph_document;

GraphDocument::GraphDocument(float displayWidth, float displayHeight) {
    m_viewModel.setModel(&m_model);
    m_viewModel.updateSceneSize(displayWidth, displayHeight);
}

GraphDocument::GraphDocument(GraphDocument&& rhs) noexcept
    : m_model(std::move(rhs.m_model)),
      m_viewModel(std::move(rhs.m_viewModel)),
      m_path(std::move(rhs.m_path)) {
    m_viewModel.setModel(&m_model);
}

GraphDocument& GraphDocument::operator=(GraphDocument&& rhs) noexcept {
    if (this != &rhs) {
        m_model = std::move(rhs.m_model);
        m_viewModel = std::move(rhs.m_viewModel);
        m_path = std::move(rhs.m_path);

        m_viewModel.setModel(&m_model);
    }

    return *this;
}

const char* GraphDocument::getName() const {
    if (m_path.empty()) {
        return "Unsaved Graph";
    }

    const auto pos = m_path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return m_path.c_str() + pos + 1;
    }

    return m_path.c_str();
}
