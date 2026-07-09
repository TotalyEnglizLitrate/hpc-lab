#include "sort.hpp"

void stalin_sort(UnsignedIntVec& vec) {
    if (vec.empty()) return;

    UnsignedIntVec out;
    out.reserve(vec.size());

    int max_so_far = vec[0];
    out.push_back(vec[0]);

    for (size_t i = 1; i < vec.size(); ++i) {
        int x = vec[i];
        if (x >= max_so_far) {
            max_so_far = x;
            out.push_back(x);
        }
    }

    vec = std::move(out);
}