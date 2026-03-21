module;
#include <pch.h>

export module graph_document_listener;

export import graph_document;

export class IGraphDocumentListener {
   public:
    virtual ~IGraphDocumentListener() = default;

    virtual void onDocumentChanged(GraphDocument& document) = 0;
};
