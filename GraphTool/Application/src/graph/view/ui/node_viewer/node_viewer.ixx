module;
#include <pch.h>

export module node_viewer;

import graph_model;
import graph_view_model;

import node_states;
import algorithm;

export class NodeViewer : public IGraphViewModelListener {
   public:
    void render(const GraphModel* model, GraphViewModel* viewModel);

    void openAddEdgePopup();

    bool& isOpen();

   protected:
    void onFullDataUpdate() override {}

    void onNodeSelected(NodeIndex_t nodeIndex) override;
    void onNodeDeselected(NodeIndex_t nodeIndex) override {}

    void onNodeHover(NodeIndex_t nodeIndex) override {}
    void onNodeUnhover(NodeIndex_t nodeIndex) override {}

    void onNodeAdded(NodeIndex_t nodeIndex) override;
    void onNodeAddedToVisibleData(NodeIndex_t nodeIndex, uint32_t lookupIndex,
                                  VisibleData& visibleData) override {}

    void onNodeStateChange(NodeIndex_t nodeIndex, NodeState newState,
                           AlgorithmType algorithmType) override {}

    void onAlgorithmStarted() override {}
    void onAlgorithmAborted() override {}

    void onAlgorithmPseudocodeEvent(const std::string_view event) override {}

   private:
    void drawNodeViewer();
    void drawAddEdgePopup();
    void drawChangeIDPopup();
    void drawChangeWeightPopup();

    void drawTextCentered(const char* fmt, ...);

    const GraphModel* m_model{nullptr};
    GraphViewModel* m_viewModel{nullptr};

    NodeIndex_t m_selectedNode{INVALID_NODE};
    NodeIndex_t m_shouldScrollToNode{INVALID_NODE};

    bool m_isOpen{true};

    bool m_addEdgePopupOpen{false};
    bool m_changeIDPopupOpen{false};
    bool m_changeWeightPopupOpen{false};
};
