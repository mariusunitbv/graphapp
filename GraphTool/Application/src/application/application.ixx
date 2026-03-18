module;
#include <pch.h>

export module application;

import graph_ui;
import graph_renderer;
import graph_document_handler;

export class Application {
   public:
    static Application& get();

    void initialize();
    void run();
    void quit();

   private:
    void setupWindowIcon();
    void setupFonts(float scale);

    void onSwitchedDocument(GraphDocument& graphDocument);

    const char* getGlslVersion() const;

    void handleMaximizationShortcut();
    void limitFps(Uint64 frameStart);

    Application() = default;

    SDL_Window* m_window{nullptr};
    SDL_GLContext m_glContext{nullptr};

    GraphUI m_graphUI{};
    GraphRenderer m_graphRenderer{};
    GraphViewSettings m_graphViewSettings{};
    GraphDocumentHandler m_documentHandler{};

    std::vector<GraphDocument> m_openDocuments{};
    size_t m_currentDocumentIndex{0};
};
