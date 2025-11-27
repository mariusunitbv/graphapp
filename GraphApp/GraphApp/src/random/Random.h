#pragma once

class Random {
   public:
    static Random& get();

    int getInt(int min, int max);
    size_t getSize(size_t min, size_t max);

    std::mt19937& getEngine();

   private:
    Random();

    std::mt19937 m_engine;
};
