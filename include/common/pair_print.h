#pragma once

#include <ostream>
#include "pair.h"

template<class TKey, class TValue>
std::ostream& operator<<(std::ostream& os, const Pair<TKey, TValue>& pair) {
    os << "(" << pair.key << ":" << pair.value << ")";
    return os;
}