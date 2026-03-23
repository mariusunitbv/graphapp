module;
#include <pch.h>

export module file_view;

export import graph_document_handler;

export class FileView {
   public:
    void initialize(GraphDocumentHandler* docHandler);
    void render();

    bool& isOpen();
    void refreshRootFolder();

    void setOpenedRootFolder(const std::string& folder);
    const std::string& getOpenedRootFolder() const;

    void openFolderDialog();

   private:
    void renderOSMPopup();

    void drawTextCentered(const char* fmt, ...);

    struct FileEntry {
        enum class Type : uint8_t { FILE, PBF_FILE, FOLDER };

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

    bool m_isOpen{true};
    bool m_isOpenOSMPopup{false};

    GraphDocumentHandler* m_documentHandler{nullptr};
    std::string m_openedRootFolder{Constants::assetsFolder};
    std::vector<FileEntry> m_filesInRootFolder;
    FileEntry* m_selectedFileEntry{nullptr};
};
