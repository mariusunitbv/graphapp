module;
#include <pch.h>

export module application;

import graph_view;
import graph_model;
import graph_view_model;

export class Application {
   public:
    static Application& get();

    void initialize();
    void run();
    void quit();

   private:
    void initializeGraph(float width, float height);
    const char* getGlslVersion() const;

    void handleMaximizationShortcut();
    void addNodesForTesting();

    Application() = default;

    SDL_Window* m_window{nullptr};
    SDL_GLContext m_glContext{nullptr};

    GraphModel m_graphModel{};
    GraphViewModel m_graphViewModel{};
    GraphView m_graphView{};
};
