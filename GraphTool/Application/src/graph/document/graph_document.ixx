module;
#include <pch.h>

export module graph_document;

export import graph_model;
export import graph_view_model;

export struct GraphDocument {
    GraphDocument(float displayWidth, float displayHeight);

    GraphDocument(const GraphDocument&) = delete;
    GraphDocument& operator=(const GraphDocument&) = delete;

    GraphDocument(GraphDocument&& rhs) noexcept;
    GraphDocument& operator=(GraphDocument&& rhs) noexcept;

    const char* getName() const;

    GraphModel m_model{};
    GraphViewModel m_viewModel{};

    std::string m_path;
};
