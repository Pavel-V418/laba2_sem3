#pragma once

// Общие помощники для тестов

#include "index/linear_search.h"

// Равны ли два списка позиций как множества (индексы хеш-таблицы отдают позиции
// в произвольном порядке)
inline bool same_positions(const Sequence<int>& a, const Sequence<int>& b) {
    return same_position_sets(a, b);
}
