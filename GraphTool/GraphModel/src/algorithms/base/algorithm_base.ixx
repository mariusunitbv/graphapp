module;
#include <pch.h>

export module algorithm_base;

export import graph_model;

export import algorithm;
export import algorithm_listener;

export class AlgorithmBase : public IAlgorithm {
   public:
    void setModel(GraphModel* model) override;
    void addListener(IAlgorithmListener* listener) override;
    void removeListeners() override;
    void setShouldNotifyListeners(bool shouldNotify) override;

    void restart() override;
    void finish() override;

    void step() override;
    void undo(int stepsToUndo) override;
    bool isFinished() const override;

    void setSourceNode(NodeIndex_t sourceNode) override;
    void setTargetNode(NodeIndex_t targetNode) override;

    const std::vector<std::pair<NodeIndex_t, NodeIndex_t>>& getHighlightedEdges() const override;

    void setNodesState(NodeState newState);
    void setNodeState(NodeIndex_t nodeIndex, NodeState newState);
    NodeState getNodeState(NodeIndex_t nodeIndex) const;

    void notifyAlgorithmFinished();
    void notifyNodeStateChanged(NodeIndex_t nodeIndex, NodeState newState);
    void notifyPseudocodeEvent(const std::string_view event);

   protected:
    virtual void restartAlgorithm() = 0;
    virtual bool stepAlgorithm() = 0;

    GraphModel* m_model{nullptr};

    NodeIndex_t m_sourceNode{INVALID_NODE};
    NodeIndex_t m_targetNode{INVALID_NODE};

    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_highlightedEdges;

   private:
    std::vector<IAlgorithmListener*> m_listeners;
    int m_currentStep{0};
    bool m_finished{false};
    bool m_shouldNotifyListeners{true};
};
