module;
#include <pch.h>

module application;

import graph_common;

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

    m_window = SDL_CreateWindow("Graph Tool " GAPP_VERSION, (int)startWidth, (int)startHeight,
                                windowFlags);
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
    SDL_SetWindowSize(m_window, (int)startWidth, (int)startHeight);
    SDL_ShowWindow(m_window);

#ifndef __EMSCRIPTEN__
    setupWindowIcon();
#else
    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, m_window, true,
        ([](int eventType, const EmscriptenUiEvent* uiEvent, void* userData) {
            const int w = uiEvent->windowInnerWidth;
            const int h = uiEvent->windowInnerHeight;

            SDL_Window* window = static_cast<SDL_Window*>(userData);
            SDL_SetWindowSize(window, w, h);

            return EM_TRUE;
        }));
#endif

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    io.IniFilename = Constants::imguiIniFile;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

    setupFonts(scale);

    style.ScaleAllSizes(scale);
    style.AntiAliasedFill = style.AntiAliasedLines = true;
    ImGui::StyleColorsClassic();

    ImGui_ImplSDL3_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);

    m_graphUI.initialize(&m_graphViewSettings, &m_documentHandler);
    m_graphRenderer.initialize(&m_graphViewSettings);

    m_documentHandler.initialize(&m_graphRenderer);
    m_documentHandler.addListener(this);

    m_isRunning = true;
}

void Application::run() {
    auto& io = ImGui::GetIO();

#ifndef __EMSCRIPTEN__
    while (m_isRunning) {
        const auto frameStart = SDL_GetPerformanceCounter();

        static bool fullscreenState = false;
        if (fullscreenState != m_graphUI.isFullScreen()) {
            fullscreenState = !fullscreenState;
            SDL_SetWindowFullscreen(m_window, fullscreenState);
        }
#endif

        static int vsyncState = -1;
        if (vsyncState != m_graphUI.getVsyncMode()) {
            vsyncState = m_graphUI.getVsyncMode();
            SDL_GL_SetSwapInterval(m_graphUI.getVsyncMode());
        }

        if (m_openDocuments.empty()) {
            m_documentHandler.addEmptyDocument(m_openDocuments);
        }

        auto& openDocument = m_openDocuments[m_currentDocumentIndex];
        GraphModel* currentModel = &openDocument.m_model;
        GraphViewModel* currentViewModel = &openDocument.m_viewModel;

        m_graphUI.preRenderUpdate(currentModel, currentViewModel);
        m_graphRenderer.preRenderUpdate(currentModel, currentViewModel);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

#ifndef __EMSCRIPTEN__
            if ((event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 event.window.windowID == SDL_GetWindowID(m_window)) ||
                event.type == SDL_EVENT_QUIT) {
                m_isRunning = false;
                break;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_F:
                        if (!m_graphUI.isFocusOnUI()) {
                            handleMaximizationShortcut();
                        }
                        break;
                    case SDLK_F11:
                        m_graphUI.toggleFullScreen();
                        break;
                    case SDLK_W:
                        if (SDL_GetModState() & SDL_KMOD_CTRL) {
                            m_documentHandler.scheduleCloseDocument(m_currentDocumentIndex);
                        }
                        break;
                }
            }

            if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
                m_graphUI.refreshRootFolder();
            }
#endif

            currentViewModel->onSDLEvent(event, m_graphUI.isFocusOnUI());
            m_graphUI.onSDLEvent(event);
        }

        currentViewModel->preRenderUpdate();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        m_graphRenderer.render();
        m_graphUI.render(m_openDocuments, m_currentDocumentIndex);

        ImGui::Render();

        auto scale = 1.f;

#ifdef __EMSCRIPTEN__
        scale = emscripten_get_device_pixel_ratio();
#endif

        glViewport(0, 0, (int)(io.DisplaySize.x * scale), (int)(io.DisplaySize.y * scale));
        glClear(GL_COLOR_BUFFER_BIT);

        m_graphRenderer.renderNative();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(m_window);

        m_documentHandler.processTasks(m_openDocuments, m_currentDocumentIndex);

#ifndef __EMSCRIPTEN__
        limitFps(frameStart);
    }

    quit();
#endif
}

bool Application::isRunning() const { return m_isRunning; }

void Application::onDocumentChanged(GraphDocument& graphDocument) {
    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);

    graphDocument.m_viewModel.updateSceneSize(static_cast<float>(width),
                                              static_cast<float>(height));
    graphDocument.m_viewModel.refreshVisibleData();
}

void Application::quit() {
    m_graphUI.saveSettingsToJsonHelper(m_openDocuments, m_currentDocumentIndex);

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

    const auto error = lodepng::decode(imageData, width, height, Constants::appIconPath);
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

    ImFontConfig config{};

    config.FontBuilderFlags =
        ImGuiFreeTypeBuilderFlags_Bitmap | ImGuiFreeTypeBuilderFlags_Monochrome;
    config.PixelSnapH = true;
    config.OversampleH = config.OversampleV = 1;

    io.Fonts->AddFontFromFileTTF(Constants::defaultFontPath, 26.f, &config,
                                 io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF(Constants::defaultFontPath, 13.f, &config,
                                 io.Fonts->GetGlyphRangesDefault());

    io.Fonts->Build();
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

    auto maxFps = m_graphUI.getMaxFps();
    if (minimized || unfocused) {
        maxFps = 5;
    }

    if (m_graphUI.isFpsLimitEnabled() || minimized || unfocused) {
        const auto targetFrameTime = 1.0 / maxFps;
        const auto elapsed = (double)(SDL_GetPerformanceCounter() - frameStart) / frequency;
        if (elapsed < targetFrameTime) {
            const auto remaining = targetFrameTime - elapsed;
            SDL_Delay((Uint32)(remaining * 1000.0));
        }
    }
}
