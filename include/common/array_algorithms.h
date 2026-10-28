#pragma once

// Собственные алгоритмы над DynamicArray (std::sort / std::shuffle запрещены в ядре).
// Less — любой callable вида bool(const T&, const T&): функция, функтор или лямбда.
#include <memory>
#include <random>
#include "LR2/dynamicArray.h"
#include "LR2/sequence.h"
#include "LR2/mutableArraySequence.h"

namespace detail {

    // Сортировка: рекурсивная реализация
    template<class T, class Less>
    void merge_sort_impl(DynamicArray<T>& arr, DynamicArray<T>& buffer, int low, int high, Less& less) {
        if (high - low < 2)
            return;

        int mid = low + (high - low) / 2;
        merge_sort_impl(arr, buffer, low, mid, less);
        merge_sort_impl(arr, buffer, mid, high, less);

        // Если половины уже стоят в правильном порядке - слияние не нужно
        // (благодаря ей на отсортированных данных сортировка работает за O(n))
        if (!less(arr.get(mid), arr.get(mid - 1)))
            return;

        int i = low; // по левой половине
        int j = mid; // по правой половие
        int k = low; // куда писать в buffer
        while (i < mid && j < high) {
            // берём из правой половины только если она СТРОГО меньше, чтобы сортировка была устойчива
            if (less(arr.get(j), arr.get(i)))
                buffer.set(arr.get(j++), k++);
            else
                buffer.set(arr.get(i++), k++);
        }
        while (i < mid)
            buffer.set(arr.get(i++), k++);
        while (j < high)
            buffer.set(arr.get(j++), k++);

        for (int t = low; t < high; t++)
            arr.set(buffer.get(t), t);
    }

}

// готовая обёртка
template<class T, class Less>
void merge_sort(DynamicArray<T>& arr, Less less) {
    int n = arr.get_size();
    if (n < 2)
        return;

    DynamicArray<T> buffer(n);
    detail::merge_sort_impl(arr, buffer, 0, n, less);
}

// Разворот массива на месте
template<class T>
void reverse(DynamicArray<T>& arr) {
    int i = 0, j = arr.get_size() - 1;
    while (i < j) {
        T tmp = arr.get(i);
        arr.set(arr.get(j), i);
        arr.set(tmp, j);
        i++;
        j--;
    }
}

// Перемешивание Фишера–Йейтса: каждая перестановка равновероятна
template<class T>
void shuffle(DynamicArray<T>& arr, std::mt19937& rng) {
    for (int i = arr.get_size() - 1; i > 0; i--) { // не i >= 0, т.к. при 0 ничего шафлить не надо
        std::uniform_int_distribution<int> dist(0, i);
        int j = dist(rng);
        if (i != j) {
            T tmp = arr.get(i);
            arr.set(arr.get(j), i);
            arr.set(tmp, j);
        }
    }
}

// Отсортированная копия последовательности (исходная не меняется)
template<class T, class Less>
std::unique_ptr<MutableArraySequence<T>> sorted_copy(const Sequence<T>& seq, Less less) {
    DynamicArray<T> arr(seq.get_length()); // создали пустой массив, куда будем копировать элементы из seq

    IEnumerator<T>* it = seq.get_enumerator();
    int i = 0;
    while (it->has_more_elements())
        arr.set(it->next(), i++);
    delete it;

    merge_sort(arr, less);

    std::unique_ptr<MutableArraySequence<T>> result = std::make_unique<MutableArraySequence<T>>();
    for (int k = 0; k < arr.get_size(); k++)
        result->append(arr.get(k));
    return result;
}

// Проверка упорядоченности (для тестов)
template<class T, class Less>
bool is_sorted(const Sequence<T>& seq, Less less) {
    for (int i = 1; i < seq.get_length(); i++)
        if (less(seq.get(i), seq.get(i - 1)))
            return false;
    return true;
}

