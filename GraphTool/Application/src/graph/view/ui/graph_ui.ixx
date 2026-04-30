module;
#include <pch.h>

export module graph_ui;

export import graph_view_settings;

import log_view;
import file_view;
import node_viewer;
import pseudocode_view;

import graph_model;
import graph_view_model;
import graph_document_handler;

export class GraphUI {
   public:
    ~GraphUI();

    void initialize(GraphViewSettings* viewSettings, GraphDocumentHandler* docHandler);
    void preRenderUpdate(const GraphModel* model, GraphViewModel* viewModel);
    void onSDLEvent(const SDL_Event& event);

    void render();

    bool isFocusOnUI() const;

    enum class UITheme : uint8_t {
        IMGUI_WHITE,
        IMGUI_DARK,
        IMGUI_CLASSIC,
        CORPORATE_GREY,
        CATPPUCCIN,
        CHERRY,
        VGUI,
        UITHEME_COUNT,
    };

    enum class GraphTheme_t : uint8_t { DARK, LIGHT, CUSTOM };

    void toggleFullScreen() { m_appFullScreen = !m_appFullScreen; }

    auto viewSettings() { return m_viewSettings; }

    auto& isSettingsOpen() { return m_isSettingsOpen; }
    auto& isFullScreen() { return m_appFullScreen; }
    auto& fileViewOpen() { return m_fileView.isOpen(); }
    auto& logsOpen() { return m_logView.isOpen(); }
    auto& inspectorOpen() { return m_inspectorOpen; }
    auto& nodeViewerOpen() { return m_nodeViewer.isOpen(); }
    auto& algorithmPickerOpen() { return m_algorithmsPickerOpen; }

    auto& currentTheme() { return m_currentTheme; }
    auto& currentGraphTheme() { return m_currentGraphTheme; }

    auto& isFpsLimitEnabled() { return m_isFpsLimitEnabled; }
    auto& getMaxFps() { return m_maxFps; }
    auto& getVsyncMode() { return m_vsyncMode; }

    void setOpenedRootFolder(const std::string& folder) { m_fileView.setOpenedRootFolder(folder); }
    const auto& getOpenedRootFolder() const { return m_fileView.getOpenedRootFolder(); }

    NodeViewer& getNodeViewer() { return m_nodeViewer; }
    PseudocodeView& getPseudocodeView() { return m_pseudocodeView; }

   private:
    void initializeTextures();

    void setupDockSpace();
    void drawMenuBar();
    void drawOpenedTabs();
    void drawStatusBar();
    void drawDeleteConfirmationDialog();
    void drawCenterOnNodeDialog();
    void drawInspector();
    void drawSettings();

    void drawAlgorithmsPicker();
    void drawPlaybackControls(std::function<void(int)> on_click);

    void drawUnfocusedBackground(ImDrawList* drawList);
    void drawAddNodesText(ImDrawList* drawList);
    void drawVersion(ImDrawList* drawList);
    void drawWatermark(ImDrawList* drawList);

    void drawTextCentered(const char* fmt, ...);

    void onThemeSwitched();
    void themeCorporateGrey();
    void themeCatppuccin();
    void themeCherry();
    void themeVGUI();

    void onGraphThemeSwitched();
    void graphThemeLight();

    void newDocument();
    void openDocument();
    void saveDocument();
    void saveDocumentAs();

    const GraphModel* m_model{nullptr};
    GraphViewModel* m_viewModel{nullptr};
    GraphViewSettings* m_viewSettings{nullptr};
    GraphDocumentHandler* m_documentHandler{nullptr};

    bool m_isDeleteDialogOpen{false};
    bool m_isCenterOnNodeDialogOpen{false};
    bool m_isSettingsOpen{false};
    bool m_appFullScreen{false};
    bool m_inspectorOpen{true};
    bool m_algorithmsPickerOpen{true};

    ImGuiStyle m_defaultStyle;
    UITheme m_currentTheme{UITheme::IMGUI_CLASSIC};
    static constexpr std::array<std::string_view, (size_t)UITheme::UITHEME_COUNT> m_themeNames{
        "Light", "Dark", "Classic", "Grey", "Catppuccin", "Cherry", "VGUI",
    };

    GraphTheme_t m_currentGraphTheme{GraphTheme_t::DARK};
    static constexpr std::array<std::string_view, 3> m_graphThemeNames{"Dark", "Light", "Custom"};

#ifndef __EMSCRIPTEN__
    bool m_isFpsLimitEnabled{true};
    int m_maxFps{120};
#else
    static constexpr int m_maxFps{0};
    static constexpr bool m_isFpsLimitEnabled{false};
#endif

    int m_vsyncMode{0};

    ImVec2 m_sceneViewPos{};
    ImVec2 m_sceneViewSize{};
    GLuint m_unitbvLogoTexture{};

    FileView m_fileView;
    LogView m_logView;
    NodeViewer m_nodeViewer;
    PseudocodeView m_pseudocodeView;
};
