#pragma once

// Наивный поиск полным перебором коллекции - O(n) на каждый запрос.
// Нужен для двух вещей:
//   - автоматической и ручной проверки корректности индексов: результат индекса должен совпасть с перебором;
//   - сравнения по времени: "индекс vs без индекса".
// Позиции возвращаются по возрастанию.

#include <memory>
#include "../common/comparators.h"
#include "../common/array_algorithms.h"
#include "../LR2/dynamicArray.h"
#include "../LR2/sequence.h"
#include "../LR2/mutableArraySequence.h"

// Равны ли два списка позиций как МНОЖЕСТВА. Хеш-таблица отдаёт позиции диапазона
// в произвольном порядке, поэтому сравниваем отсортированные копии: O(k log k).
inline bool same_position_sets(const Sequence<int>& a, const Sequence<int>& b) {
    if (a.get_length() != b.get_length())
        return false;

    DynamicArray<int> x(a.get_length());
    DynamicArray<int> y(b.get_length());
    for (int i = 0; i < a.get_length(); i++) {
        x.set(a.get(i), i);
        y.set(b.get(i), i);
    }
    auto less = [](int p, int q) { return p < q; };
    merge_sort(x, less);
    merge_sort(y, less);

    for (int i = 0; i < x.get_size(); i++)
        if (x.get(i) != y.get(i))
            return false;
    return true;
}
// проходит всю коллекцию и возвращает позиции подходящих объектов по возрастанию O(n)
template<class TItem, class TKey, class F>
std::unique_ptr<MutableArraySequence<int>> linear_find(const Sequence<TItem>& data, F extractor,const TKey& key) {
    auto result = std::make_unique<MutableArraySequence<int>>();
    std::unique_ptr<IEnumerator<TItem>> it(data.get_enumerator());
    int position = 0;
    while (it->has_more_elements()) {
        if (extractor(it->next()) == key)
            result->append(position);
        position++;
    }
    return result;
}

template<class TItem, class TKey, class F, class Less = KeyLess<TKey>>
std::unique_ptr<MutableArraySequence<int>> linear_find_range(const Sequence<TItem>& data, F extractor,const TKey& from, const TKey& to,Less less = Less()) {
    auto result = std::make_unique<MutableArraySequence<int>>();
    std::unique_ptr<IEnumerator<TItem>> it(data.get_enumerator());
    int position = 0;
    while (it->has_more_elements()) {
        TKey key = extractor(it->next());
        if (!less(key, from) && !less(to, key))
            result->append(position);
        position++;
    }
    return result;
}
