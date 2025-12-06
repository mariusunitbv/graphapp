#include <pch.h>

#include "Random.h"

Random::Random() : m_engine{std::random_device()()} {}

Random& Random::get() {
    static Random instance;
    return instance;
}

int Random::getInt(int min, int max) {
    std::uniform_int_distribution dist(min, max);
    return dist(m_engine);
}

size_t Random::getSize(size_t min, size_t max) {
    std::uniform_int_distribution dist(min, max);
    return dist(m_engine);
}

QRgb Random::getColor() {
    std::uniform_int_distribution dist(30, 255);
    return qRgb(dist(m_engine), dist(m_engine), dist(m_engine));
}

std::mt19937& Random::getEngine() { return m_engine; }
