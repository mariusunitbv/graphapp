module;
#include <pch.h>

export module algorithm_factory;

export import algorithm;

export class AlgorithmFactory {
   public:
    static std::unique_ptr<IAlgorithm> createAlgorithm(AlgorithmType type);
};
