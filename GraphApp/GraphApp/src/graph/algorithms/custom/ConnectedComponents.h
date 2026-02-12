#pragma once

#include "../traversals/DepthFirstTotalTraversal.h"

class ConnectedComponents final : public DepthFirstTotalTraversal {
    Q_OBJECT

   public:
    ConnectedComponents(Graph* graph);

    void start() override;
    bool step() override;
    void showPseudocodeForm() override;

   private:
    void onFinishedAlgorithm() override;
    void updateAlgorithmInfoText() const override;
    void resetForUndo() override;

    void colorCurrentConnectedComponent();

    void onPickedNewStartNode(NodeIndex_t startNode);
    void onVisitedNode(NodeIndex_t node);
    void onAnalyzingNode(NodeIndex_t node);
    void onAnalyzedNode(NodeIndex_t node);

    std::vector<QRgb> m_componentColors;
    std::vector<NodeIndex_t> m_currentConnectedComponent;
    std::vector<std::vector<NodeIndex_t>> m_connectedComponents;
};
