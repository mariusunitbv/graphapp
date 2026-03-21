module;
#include <pch.h>

export module application;

import graph_ui;
import graph_renderer;
import graph_document_handler;
import graph_document_listener;

export class Application : public IGraphDocumentListener {
   public:
    static Application& get();

    void initialize();

    void run();
    bool isRunning() const;

    void quit();

   protected:
    void onDocumentChanged(GraphDocument& graphDocument) override;

   private:
    void setupWindowIcon();
    void setupFonts(float scale);

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

    bool m_isRunning{false};
};
