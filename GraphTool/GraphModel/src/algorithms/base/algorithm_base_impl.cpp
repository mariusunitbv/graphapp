module;
#include <pch.h>

module algorithm_base;

void AlgorithmBase::setModel(GraphModel* model) { m_model = model; }

void AlgorithmBase::addListener(IAlgorithmListener* listener) { m_listeners.push_back(listener); }

void AlgorithmBase::restart() {
    if (m_currentStep <= 0) {
        return;
    }

    m_finished = false;
    m_currentStep = 0;
    m_highlightedEdges.clear();

    restartAlgorithm();
}

void AlgorithmBase::finish() {
    m_shouldInstantlyFinish = true;
    while (!isFinished()) {
        step();
    }
    m_shouldInstantlyFinish = false;
}

void AlgorithmBase::step() {
    if (m_finished) {
        return;
    }

    if (!stepAlgorithm()) {
        m_finished = true;
        notifyAlgorithmFinished();
    }

    ++m_currentStep;
}

void AlgorithmBase::undo(int stepsToUndo) {
    if (m_currentStep <= 0) {
        return;
    }

    int stepsToRedo = m_currentStep - stepsToUndo;
    restart();

    for (int i = 0; i < stepsToRedo; ++i) {
        step();
    }
}

bool AlgorithmBase::isFinished() const { return m_finished; }

void AlgorithmBase::setSourceNode(NodeIndex_t sourceNode) { m_sourceNode = sourceNode; }

void AlgorithmBase::setTargetNode(NodeIndex_t targetNode) { m_targetNode = targetNode; }

const std::vector<std::pair<NodeIndex_t, NodeIndex_t>>& AlgorithmBase::getHighlightedEdges() const {
    return m_highlightedEdges;
}

void AlgorithmBase::setNodesState(NodeState newState) {
    for (NodeIndex_t nodeIndex = 0; nodeIndex < m_model->getNodeCount(); ++nodeIndex) {
        setNodeState(nodeIndex, newState);
    }
}

void AlgorithmBase::setNodeState(NodeIndex_t nodeIndex, NodeState newState) {
    const auto node = m_model->getNode(nodeIndex);

    if (newState == NodeState::NONE) {
        node->clearColor();
    }

    node->setState(static_cast<uint8_t>(newState));
    notifyNodeStateChanged(nodeIndex, newState);
}

NodeState AlgorithmBase::getNodeState(NodeIndex_t nodeIndex) const {
    return static_cast<NodeState>(m_model->getNode(nodeIndex)->getState());
}

void AlgorithmBase::notifyAlgorithmFinished() {
    for (auto* listener : m_listeners) {
        listener->onAlgorithmFinish();
    }
}

void AlgorithmBase::notifyNodeStateChanged(NodeIndex_t nodeIndex, NodeState newState) {
    if (m_shouldInstantlyFinish) {
        return;
    }

    for (auto* listener : m_listeners) {
        listener->onNodeStateChange(nodeIndex, newState);
    }
}

void AlgorithmBase::notifyPseudocodeEvent(const std::string_view event) {
    if (m_shouldInstantlyFinish) {
        return;
    }

    for (auto* listener : m_listeners) {
        listener->onAlgorithmPseudocodeEvent(event);
    }
}
