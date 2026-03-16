module;
#include <pch.h>

export module graph_view_model_listener;

import graph_model_defines;
import graph_view_model_defines;

export class IGraphViewModelListener {
   public:
    virtual ~IGraphViewModelListener() = default;

    virtual void onFullDataUpdate() = 0;

    virtual void onNodeSelected(NodeIndex_t nodeIndex) = 0;
    virtual void onNodeDeselected(NodeIndex_t nodeIndex) = 0;

    virtual void onNodeHover(NodeIndex_t nodeIndex) = 0;
    virtual void onNodeUnhover(NodeIndex_t nodeIndex) = 0;

    virtual void onNodeAdded(NodeIndex_t nodeIndex) = 0;
    virtual void onNodeAddedToVisibleData(NodeIndex_t nodeIndex, uint32_t lookupIndex,
                                          VisibleData& visibleData) = 0;
};
