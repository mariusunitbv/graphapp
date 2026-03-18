module;
#include <pch.h>

export module application;

import graph_document;
import graph_view;

export class Application {
   public:
    static Application& get();

    void initialize();
    void run();
    void quit();

   private:
    void setupWindowIcon();
    void setupFonts(float scale);

    void createNewDocument(float width, float height);
    void onSwitchedDocument(GraphDocument& graphDocument);

    const char* getGlslVersion() const;

    void handleMaximizationShortcut();
    void limitFps(Uint64 frameStart);

    Application() = default;

    SDL_Window* m_window{nullptr};
    SDL_GLContext m_glContext{nullptr};

    GraphView m_graphView{};
    GraphViewSettings m_graphViewSettings{};

    std::vector<GraphDocument> m_openDocuments{};
    size_t m_currentDocumentIndex{0};
};
