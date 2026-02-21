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

    const auto startWidth = 1024 * scale;
    const auto startHeight = 576 * scale;

    m_window = SDL_CreateWindow("Graph Tool", (int)startWidth, (int)startHeight, windowFlags);
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
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(m_window);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    io.IniFilename = io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

    style.ScaleAllSizes(scale);
    style.AntiAliasedFill = style.AntiAliasedLines = true;

    this->initializeGraph(startWidth, startHeight);
    ImGui::StyleColorsClassic();

    ImGui_ImplSDL3_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);
}

void Application::run() {
    bool done = false;

    const auto frequency = SDL_GetPerformanceFrequency();
    auto& io = ImGui::GetIO();

    while (!done) {
        const auto frameStart = SDL_GetPerformanceCounter();

        static bool fullscreenState = false;
        if (fullscreenState != m_graphView.isFullScreen()) {
            fullscreenState = !fullscreenState;
            SDL_SetWindowFullscreen(m_window, fullscreenState);
        }

        static int vsyncState = -1;
        if (vsyncState != m_graphView.getVsyncMode()) {
            vsyncState = m_graphView.getVsyncMode();
            SDL_GL_SetSwapInterval(m_graphView.getVsyncMode());
        }

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
                switch (event.key.key) {
                    case SDLK_F:
                        handleMaximizationShortcut();
                        break;
                    case SDLK_F11:
                        m_graphView.toggleFullScreen();
                        break;
                }
            }

            m_graphViewModel.onSDLEvent(event);
        }

        m_graphViewModel.preRenderUpdate();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        m_graphView.renderUI();
        ImGui::Render();

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);

        m_graphView.renderScene();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(m_window);

        if (m_graphView.getMaxFps() != 0) {
            const auto targetFrameTime = 1.0 / m_graphView.getMaxFps();
            const auto elapsed = (double)(SDL_GetPerformanceCounter() - frameStart) / frequency;
            if (elapsed < targetFrameTime) {
                const auto remaining = targetFrameTime - elapsed;
                SDL_Delay((Uint32)(remaining * 1000.0));
            }
        }
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

void Application::initializeGraph(float width, float height) {
    m_graphViewModel.initialize(&m_graphModel, &m_graphView, width, height);
    m_graphView.initialize(&m_graphModel, &m_graphViewModel);

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
    // return;
    constexpr float start = -5000.f;
    constexpr float end = -start;
    constexpr float step = NODE_RADIUS * 2.f;

    constexpr size_t stepsPerAxis = static_cast<size_t>((end - start) / step) + 1;
    constexpr size_t nodeCount = stepsPerAxis * stepsPerAxis;

    static_assert(nodeCount < NODE_LIMIT, "Node count exceeds limits");

    m_graphModel.reserveNodes(nodeCount);
    std::cout << "Adding " << nodeCount << " nodes for testing..." << std::endl;

    m_graphModel.beginBulkInsert();
    for (float y = start; y <= -start; y += step) {
        for (float x = start; x <= -start; x += step) {
            m_graphModel.addNode({x, y});
        }
    }
    m_graphModel.endBulkInsert();

    std::cout << "Finished adding nodes." << std::endl;
}
