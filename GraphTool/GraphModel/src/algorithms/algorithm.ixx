module;
#include <pch.h>

export module algorithm;

import algorithm_listener;
import graph_model;

export enum class AlgorithmType {
    BREADTH_FIRST_SEARCH = 0,
    DEPTH_FIRST_SEARCH,

    ALGORITHM_TYPE_MAX
};

export class IAlgorithm {
   public:
    // Using a vector of key-value pairs to represent execution information, where the key is a
    // string describing the information type and the value is the corresponding information.
    using ExecutionInfo_t = std::vector<std::pair<std::string, std::string>>;

    virtual ~IAlgorithm() = default;

    virtual AlgorithmType getType() const = 0;
    virtual const char* getName() const = 0;

    virtual void setModel(GraphModel* model) = 0;
    virtual void addListener(IAlgorithmListener* listener) = 0;

    virtual void setSourceNode(NodeIndex_t sourceNode) = 0;
    virtual void setTargetNode(NodeIndex_t targetNode) = 0;

    virtual void initialize() = 0;
    virtual void restart() = 0;

    virtual void step() = 0;
    virtual void undo(int stepsToUndo) = 0;
    virtual bool isFinished() const = 0;

    virtual ExecutionInfo_t getExecutionInfo() const = 0;
};
