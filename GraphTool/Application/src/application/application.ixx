module;
#include <pch.h>

export module application;

import graph_view;

export class Application {
   public:
    static Application& get();

    void initialize();
    void run();
    void quit();

   private:
    void initializeGraph();
    const char* getGlslVersion() const;

    void handleMaximizationShortcut();
    void addNodesForTesting();

    Application() = default;

    SDL_Window* m_window{nullptr};
    SDL_GLContext m_glContext{nullptr};

    GraphView m_graphView{};
    GraphModel m_graphModel{};
    GraphViewModel m_graphViewModel{};
};
