module;
#include <pch.h>

export module settings_manager;

import graph_ui;
import graph_document_handler;

export class SettingsManager {
   public:
    void initialize(GraphUI* graphUI, GraphDocumentHandler* documentHandler);

    void saveSettings();
    void loadSettings();

   private:
    GraphUI* m_graphUI{nullptr};
    GraphDocumentHandler* m_documentHandler{nullptr};
};
