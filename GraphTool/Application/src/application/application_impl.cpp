module;
#include <pch.h>

module application;

Application& Application::get() {
    static Application instance;
    return instance;
}

void Application::initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        GAPP_THROW(SDL_GetError());
    }

    const auto glslVersion = getGlslVersion();
    const auto scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    constexpr auto windowFlags =
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    m_window = SDL_CreateWindow("Graph Tool", static_cast<int>(800 * scale),
                                static_cast<int>(450 * scale), windowFlags);
    if (!m_window) {
        GAPP_THROW(SDL_GetError());
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        GAPP_THROW(SDL_GetError());
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        GAPP_THROW("Failed to initialize GLAD");
    }

    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(0);
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(m_window);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    io.IniFilename = io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    style.ScaleAllSizes(scale);
    style.AntiAliasedFill = style.AntiAliasedLines = true;

    this->initializeGraph();
    ImGui::StyleColorsClassic();

    ImGui_ImplSDL3_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);
}

void Application::run() {
    bool done = false;

    auto& io = ImGui::GetIO();

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if ((event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 event.window.windowID == SDL_GetWindowID(m_window)) ||
                event.type == SDL_EVENT_QUIT) {
                done = true;
                break;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_F) {
                    handleMaximizationShortcut();
                    break;
                }
            }

            m_graphView.onEvent(event);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        m_graphView.renderUI();
        ImGui::Render();

        const auto backgroundColor =
            ImGui::ColorConvertU32ToFloat4(m_graphView.getBackgroundColor());

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_graphView.renderScene();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(m_window);
    }

    quit();
}

void Application::quit() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    if (m_glContext) {
        SDL_GL_DestroyContext(m_glContext);
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
    }

    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Quit();
    }
}

void Application::initializeGraph() {
    m_graphView.initialize();
    m_graphView.setViewModel(&m_graphViewModel);
    m_graphView.setModel(&m_graphModel);

    addNodesForTesting();
}

const char* Application::getGlslVersion() const {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    return "#version 330 core";
}

void Application::handleMaximizationShortcut() {
    if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MAXIMIZED) {
        SDL_RestoreWindow(m_window);
    } else {
        SDL_MaximizeWindow(m_window);
    }
}

void Application::addNodesForTesting() {
    constexpr float start = -5000.f;
    constexpr float end = -start;
    constexpr float step = GraphView::NODE_RADIUS * 5.f;

    constexpr size_t stepsPerAxis = static_cast<size_t>((end - start) / step) + 1;
    constexpr size_t nodeCount = stepsPerAxis * stepsPerAxis;

    static_assert(nodeCount < NODE_LIMIT, "Node count exceeds size_t limits");

    m_graphModel.reserveNodes(nodeCount);
    std::cout << "Adding " << nodeCount << " nodes for testing..." << std::endl;

    for (float y = start; y <= -start; y += step) {
        for (float x = start; x <= -start; x += step) {
            m_graphModel.addNode(x, y);
        }
    }

    std::cout << "Finished adding nodes." << std::endl;
    m_graphView.buildQuadTree();
    std::cout << "Finished building quadtree." << std::endl;
}
