#include "sort.hpp"

#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>

void sleep_sort(UnsignedIntVec& vec) {
    if (vec.empty()) return;

    UnsignedIntVec out;
    out.reserve(vec.size());
    std::mutex m;

    std::vector<std::thread> threads;
    threads.reserve(vec.size());

    for (int x : vec) {
        threads.emplace_back([x, &out, &m] {
            std::this_thread::sleep_for(std::chrono::milliseconds(x));
            std::lock_guard<std::mutex> lock(m);
            out.push_back(x);
        });
    }

    for (auto& t : threads) t.join();
    std::sort(out.begin(), out.end());
    vec = std::move(out);
}