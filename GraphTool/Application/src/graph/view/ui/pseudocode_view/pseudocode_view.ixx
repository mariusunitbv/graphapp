module;
#include <pch.h>

export module pseudocode_view;

import graph_model;
import graph_view_model;
import graph_view_model_listener;

import node_states;
import algorithm;

export class PseudocodeView : public IGraphViewModelListener {
   public:
    void loadPseudocode(AlgorithmType algorithmType);
    void render(const GraphModel* model, GraphViewModel* viewModel);

    bool& isOpen();

   protected:
    void onFullDataUpdate() override {}
    void onNodeSelected(NodeIndex_t) override {}
    void onNodeDeselected(NodeIndex_t) override {}
    void onNodeHover(NodeIndex_t) override {}
    void onNodeUnhover(NodeIndex_t) override {}
    void onNodeAdded(NodeIndex_t) override {}
    void onNodeAddedToVisibleData(NodeIndex_t, uint32_t, VisibleData&) override {}
    void onNodeStateChange(NodeIndex_t, NodeState, AlgorithmType) override {}
    void onAlgorithmStarted() override {}
    void onAlgorithmAborted() override {}

    void onAlgorithmPseudocodeEvent(const std::string_view event) override;

   private:
    const GraphModel* m_model{nullptr};
    GraphViewModel* m_viewModel{nullptr};

    bool m_isOpen{true};

    struct PseudoCodeLine {
        std::string m_text;
        std::string m_event;
    };

    std::string m_currentEvent{""};
    std::vector<PseudoCodeLine> m_pseudocodeLines;
    int m_usedFontIndex{1};
};
