module;
#include <pch.h>

export module graph_ui;

export import graph_view_settings;

import graph_common;
import graph_model;
import graph_view_model;
import graph_document_handler;

export class GraphUI : public common::LogListener {
   public:
    ~GraphUI();

    void initialize(GraphViewSettings* viewSettings, GraphDocumentHandler* docHandler);
    void preRenderUpdate(const GraphModel* model, GraphViewModel* viewModel);
    void onSDLEvent(const SDL_Event& event);

    void refreshRootFolder();
    void render(const std::vector<GraphDocument>& openDocuments, size_t& currentOpenedDocument);

    bool isFocusOnUI() const;

    void toggleFullScreen() { m_appFullScreen = !m_appFullScreen; }
    bool isFullScreen() const { return m_appFullScreen; }
    bool isFpsLimitEnabled() const { return m_isFpsLimitEnabled; }
    int getMaxFps() const { return m_maxFps; }
    int getVsyncMode() const;

   protected:
    void onLogMessage(common::Logger::Level level, const std::string_view message) override;

   private:
    void initializeTextures();

    void setupDockSpace();
    void drawMenuBar();
    void drawOpenedTabs(const std::vector<GraphDocument>& openDocuments,
                        size_t& currentOpenedDocument);
    void drawLogsWindow();
    void drawStatusBar();
    void drawDeleteConfirmationDialog();
    void drawCenterOnNodeDialog();
    void drawFileView();
    void drawInspector();
    void drawSettings();

    void drawUnfocusedBackground(ImDrawList* drawList);
    void drawAddNodesText(ImDrawList* drawList);
    void drawVersion(ImDrawList* drawList);
    void drawWatermark(ImDrawList* drawList);

    void drawTextCentered(const char* fmt, ...);

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

    void onThemeSwitched();
    void themeCorporateGrey();
    void themeCatppuccin();
    void themeCherry();
    void themeVGUI();

    void onGraphThemeSwitched();
    void graphThemeLight();

    const GraphModel* m_model{nullptr};
    GraphViewModel* m_viewModel{nullptr};
    GraphViewSettings* m_viewSettings{nullptr};
    GraphDocumentHandler* m_documentHandler{nullptr};

    bool m_isDeleteDialogOpen{false};
    bool m_isCenterOnNodeDialogOpen{false};
    bool m_isSettingsOpen{false};
    bool m_appFullScreen{false};
    bool m_fileViewOpen{true};
    bool m_inspectorOpen{true};
    bool m_logsWindowOpen{true};

    ImGuiStyle m_defaultStyle;
    UITheme m_currentTheme{UITheme::IMGUI_CLASSIC};
    static constexpr std::array<std::string_view, (size_t)UITheme::UITHEME_COUNT> m_themeNames{
        "Light", "Dark", "Classic", "Grey", "Catppuccin", "Cherry", "VGUI",
    };

    int m_currentGraphTheme{0};

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

    // File View code.
    struct FileEntry {
        enum class Type : uint8_t { FILE, FOLDER };

        explicit FileEntry(const std::string& path, Type type) : m_path(path), m_type(type) {}

        const char* getName() const {
            const auto pos = m_path.find_last_of("/\\");
            if (pos != std::string::npos) {
                return m_path.c_str() + pos + 1;
            }

            return m_path.c_str();
        }

        std::string m_path;
        std::vector<FileEntry> m_children;

        Type m_type;
        bool m_isExpanded{false};
    };

    void refreshFilesInFolder(const std::string& folder, std::vector<FileEntry>& fileEntry);
    void drawFileViewHelper(const std::string& folder, std::vector<FileEntry>& fileEntry);

    std::string m_openedRootFolder{"assets/"};
    std::vector<FileEntry> m_filesInRootFolder;
    FileEntry* m_selectedFileEntry{nullptr};

    // ImGui Logger code.
    struct LineData {
        explicit LineData(uint32_t offset, uint32_t level)
            : m_lineOffset(offset), m_lineLevel(level) {}

        uint32_t m_lineOffset : 29;
        uint32_t m_lineLevel : 3;
    };

    std::vector<char> m_logBuffer;
    std::vector<LineData> m_logLines;
    ImGuiTextFilter m_logFilter;
    mutable std::shared_mutex m_logMutex;
};
