#pragma once
#include <vector>

using UnsignedIntVec = std::vector<unsigned int>;
using SortFn = void(*)(UnsignedIntVec&);

void stalin_sort(UnsignedIntVec& vec);
void bogosort(UnsignedIntVec& vec);
void sleep_sort(UnsignedIntVec& vec);