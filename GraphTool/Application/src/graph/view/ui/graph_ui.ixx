module;
#include <pch.h>

export module graph_ui;

export import graph_view_settings;

import graph_model;
import graph_view_model;
import graph_document_handler;

export class GraphUI {
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

   private:
    void initializeTextures();

    void setupDockSpace();
    void drawMenuBar();
    void drawOpenedTabs(const std::vector<GraphDocument>& openDocuments,
                        size_t& currentOpenedDocument);
    void drawStatusBar();
    void drawDeleteConfirmationDialog();
    void drawCenterOnNodeDialog();
    void drawFileView();
    void drawInspector();
    void drawSettings();

    void drawVersion(ImDrawList* drawList);
    void drawWatermark(ImDrawList* drawList);

    const GraphModel* m_model{nullptr};
    GraphViewModel* m_viewModel{nullptr};
    GraphViewSettings* m_viewSettings{nullptr};
    GraphDocumentHandler* m_documentHandler{nullptr};

    bool m_isDeleteDialogOpen{false};
    bool m_isCenterOnNodeDialogOpen{false};
    bool m_isSettingsOpen{false};
    bool m_showDemoWindow{false};
    bool m_appFullScreen{false};
    bool m_fileViewOpen{true};
    bool m_inspectorOpen{false};

#ifndef __EMSCRIPTEN__
    bool m_isFpsLimitEnabled{true};
    int m_maxFps{120};
#else
    static constexpr int m_maxFps{0};
    static constexpr bool m_isFpsLimitEnabled{false};
#endif

    int m_vsyncMode{0};

    GLuint m_unitbvLogoTexture{};

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
};
