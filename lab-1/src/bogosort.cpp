#include "sort.hpp"

#include <algorithm>
#include <random>

void bogosort(UnsignedIntVec& vec) {
    if (vec.size() < 2) return;

    static thread_local std::mt19937 rng{std::random_device{}()};

    auto sorted = [&] {
        for (size_t i = 1; i < vec.size(); ++i)
            if (vec[i - 1] > vec[i]) return false;
        return true;
    };

    while (!sorted()) {
        std::shuffle(vec.begin(), vec.end(), rng);
    }
}
