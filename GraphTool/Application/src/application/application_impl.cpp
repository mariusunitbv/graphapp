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

#ifdef __EMSCRIPTEN__
    constexpr auto windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#else
    constexpr auto windowFlags =
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif

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

#ifndef __EMSCRIPTEN__
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        GAPP_THROW("Failed to initialize GLAD");
    }
#endif

    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(m_window);

#ifndef __EMSCRIPTEN__
    setupWindowIcon();
#endif

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    io.IniFilename = io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

    setupFonts(scale);

    style.ScaleAllSizes(scale);
    style.AntiAliasedFill = style.AntiAliasedLines = true;
    ImGui::StyleColorsClassic();

    ImGui_ImplSDL3_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);

    initializeGraph(startWidth, startHeight);
}

void Application::run() {
    auto& io = ImGui::GetIO();

#ifndef __EMSCRIPTEN__
    bool done = false;
    while (!done) {
        const auto frameStart = SDL_GetPerformanceCounter();

        static bool fullscreenState = false;
        if (fullscreenState != m_graphView.isFullScreen()) {
            fullscreenState = !fullscreenState;
            SDL_SetWindowFullscreen(m_window, fullscreenState);
        }
#endif

        static int vsyncState = -1;
        if (vsyncState != m_graphView.getVsyncMode()) {
            vsyncState = m_graphView.getVsyncMode();
            SDL_GL_SetSwapInterval(m_graphView.getVsyncMode());
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

#ifndef __EMSCRIPTEN__
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
#endif

            m_graphViewModel.onSDLEvent(event);
            m_graphView.onSDLEvent(event);
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

#ifndef __EMSCRIPTEN__
        limitFps(frameStart);
    }

    quit();
#endif
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

void Application::setupWindowIcon() {
    std::vector<uint8_t> imageData;
    uint32_t width, height;

    const auto error = lodepng::decode(imageData, width, height, "assets/icon.png");
    if (error) {
        GAPP_THROW(std::string("Failed to load texture: ") + lodepng_error_text(error));
    }

    SDL_Surface* surface =
        SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, imageData.data(), width * 4);
    if (!surface) {
        GAPP_THROW(std::string("Failed to create surface: ") + SDL_GetError());
    }

    SDL_SetWindowIcon(m_window, surface);
    SDL_DestroySurface(surface);
}

void Application::setupFonts(float scale) {
    auto& io = ImGui::GetIO();

    io.Fonts->AddFontFromFileTTF("assets/JetBrainsMonoNL-Regular.ttf", 17.f * scale, nullptr,
                                 io.Fonts->GetGlyphRangesDefault());

    io.Fonts->Build();
}

void Application::initializeGraph(float width, float height) {
    m_graphViewModel.initialize(&m_graphModel, width, height);
    m_graphView.initialize(&m_graphModel, &m_graphViewModel);
}

const char* Application::getGlslVersion() const {
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    return "#version 300 es";
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    return "#version 330 core";
#endif
}

void Application::handleMaximizationShortcut() {
    if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MAXIMIZED) {
        SDL_RestoreWindow(m_window);
    } else {
        SDL_MaximizeWindow(m_window);
    }
}

void Application::limitFps(Uint64 frameStart) {
    static const auto frequency = SDL_GetPerformanceFrequency();
    const auto flags = SDL_GetWindowFlags(m_window);
    const auto minimized = flags & SDL_WINDOW_MINIMIZED;
    const auto unfocused = !(flags & SDL_WINDOW_INPUT_FOCUS);

    auto maxFps = m_graphView.getMaxFps();
    if (minimized || unfocused) {
        maxFps = 5;
    }

    if (m_graphView.isFpsLimitEnabled() || minimized || unfocused) {
        const auto targetFrameTime = 1.0 / maxFps;
        const auto elapsed = (double)(SDL_GetPerformanceCounter() - frameStart) / frequency;
        if (elapsed < targetFrameTime) {
            const auto remaining = targetFrameTime - elapsed;
            SDL_Delay((Uint32)(remaining * 1000.0));
        }
    }
}
