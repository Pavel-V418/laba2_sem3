#pragma once

// Для каждого размера n и каждого порядка генерируем ОДНУ коллекцию и на ней по очереди
// строим индекс каждой структурой. Меряем:
//   build_ms - построение индекса (мс);
//   find_us - среднее время одного точного поиска (мкс);
//   range_us - среднее время одного поиска по диапазону (мкс).
// Отдельной строкой "Linear" - поиск без индекса (полный перебор) для сравнения.

#include <string>
#include <random>
#include "timer.h"
#include "../index/index.h"
#include "../index/linear_search.h"
#include "../model/person.h"
#include "../model/person_generator.h"
#include "../model/person_attributes.h"
#include "../LR2/mutableArraySequence.h"
#include "../LR2/dynamicArray.h"
#include "../common/array_algorithms.h"

struct BenchmarkConfig { // настройка замеров
    MutableArraySequence<int> sizes;  // размеры коллекции n
    int find_queries = 1000;          // точных запросов на каждую структуру
    int range_queries = 100;          // запросов по диапазону
    int range_percent = 1;            // ширина диапазона в % от n (по отсортированным ключам)
    int linear_queries = 50;          // запросов для перебора (он медленный, хватит меньшего числа)
    // Sequence-словарь строится за O(n^2), BST на упорядоченных данных вырождается в список
    // и тоже строится за O(n^2). Выше этого n такие прогоны пропускаем (skipped = true).
    int slow_limit = 20000;
    unsigned seed = 42;
    int age_year = 2026;              // год для атрибутов с возрастом

    BenchmarkConfig() {
        sizes.append(1000);
        sizes.append(5000);
        sizes.append(10000);
        sizes.append(20000);
        sizes.append(50000);
    }
};

struct BenchmarkRow { // одна строка результатов
    std::string attribute;
    std::string structure;    // "BST", "AVL", "HashTable", "Sequence", "Linear"
    std::string order;        // "random", "sorted", "reversed"
    int n = 0;
    int distinct_keys = 0;
    int height = 0;           // высота дерева (0 - не дерево)
    double build_ms = 0;
    double find_us = 0;
    double range_us = 0;
    long long find_hits = 0;  // сколько объектов нашли - должно совпасть у всех структур
    long long range_hits = 0;
    bool skipped = false;
};

using BenchmarkRows = MutableArraySequence<BenchmarkRow>;
using BenchmarkProgress = void (*)(const BenchmarkRow& row); // принимает строку результата

inline bool is_slow_case(IndexKind kind, DataOrder order, int n, int slow_limit) {
    if (n <= slow_limit)
        return false;
    if (kind == IndexKind::Sequence)
        return true;
    return kind == IndexKind::BinaryTree && order != DataOrder::Random;
}

namespace detail {

    // Заранее выбранные ключи запросов - одинаковые для всех структур
    template<class Key>
    struct BenchmarkQueries { // заготовленные запросы
        DynamicArray<Key> exact; // ключи для точного поиска
        DynamicArray<Key> range_from;
        DynamicArray<Key> range_to;

        BenchmarkQueries(int exact_count, int range_count)
            : exact(exact_count), range_from(range_count), range_to(range_count) {}
    };

    template<class Key, class F>
    BenchmarkQueries<Key> make_queries(const Sequence<Person>& data, F extractor,const BenchmarkConfig& config, std::mt19937& rng) {
        BenchmarkQueries<Key> queries(config.find_queries, config.range_queries);
        std::uniform_int_distribution<int> position(0, data.get_length() - 1);

        for (int i = 0; i < config.find_queries; i++)
            queries.exact.set(extractor(data.get(position(rng))), i);

        // Диапазон шириной ~range_percent% коллекции: берём отсортированную КОПИЮ ключей
        // (сама коллекция не трогается) и концы диапазона на расстоянии width друг от друга.
        // Иначе случайная пара ключей даёт диапазон в треть коллекции, и время уходит
        // на копирование результата, а не на поиск.
        int n = data.get_length();
        DynamicArray<Key> sorted_keys(n);
        for (int i = 0; i < n; i++)
            sorted_keys.set(extractor(data.get(i)), i);
        merge_sort(sorted_keys, [](const Key& a, const Key& b) { return a < b; });

        int width = n * config.range_percent / 100;
        if (width < 1)
            width = 1;
        for (int i = 0; i < config.range_queries; i++) {
            int from = position(rng);
            int to = from + width < n ? from + width : n - 1;
            queries.range_from.set(sorted_keys.get(from), i);
            queries.range_to.set(sorted_keys.get(to), i);
        }
        return queries;
    }

    // замер одной структуры
    template<class Key, class F>
    BenchmarkRow measure_index(const Sequence<Person>& data, F extractor, IndexKind kind,
                               const BenchmarkQueries<Key>& queries) {
        BenchmarkRow row;
        row.structure = to_string(kind);

        Timer timer;
        Index<Person, Key> index("benchmark", data, extractor, kind);
        row.build_ms = timer.elapsed_ms();
        row.distinct_keys = index.get_key_count();
        row.height = index.get_height();

        int exact_count = queries.exact.get_size();
        timer.restart();
        for (int i = 0; i < exact_count; i++)
            row.find_hits += index.count(queries.exact.get(i));
        row.find_us = exact_count > 0 ? timer.elapsed_us() / exact_count : 0;

        int range_count = queries.range_from.get_size();
        timer.restart();
        for (int i = 0; i < range_count; i++)
            row.range_hits += index.count_range(queries.range_from.get(i), queries.range_to.get(i));
        row.range_us = range_count > 0 ? timer.elapsed_us() / range_count : 0;

        return row;
    }

    // Без индекса: каждый запрос - полный проход по коллекции
    template<class Key, class F>
    BenchmarkRow measure_linear(const Sequence<Person>& data, F extractor,
                                const BenchmarkQueries<Key>& queries, int max_queries) {
        BenchmarkRow row;
        row.structure = "Linear";

        int exact_count = queries.exact.get_size() < max_queries ? queries.exact.get_size() : max_queries;
        Timer timer;
        for (int i = 0; i < exact_count; i++)
            row.find_hits += linear_find(data, extractor, queries.exact.get(i))->get_length();
        row.find_us = exact_count > 0 ? timer.elapsed_us() / exact_count : 0;

        int range_count = queries.range_from.get_size() < max_queries ? queries.range_from.get_size() : max_queries;
        timer.restart();
        for (int i = 0; i < range_count; i++)
            row.range_hits += linear_find_range(data, extractor, queries.range_from.get(i),
                                                queries.range_to.get(i))->get_length();
        row.range_us = range_count > 0 ? timer.elapsed_us() / range_count : 0;

        return row;
    }

}

// Прогон одного атрибута по всем комбинациям: все размеры x все порядки x все структуры (+ Linear)
template<class F>
void run_attribute_benchmark(const std::string& attribute_name, F extractor, const BenchmarkConfig& config,
                             BenchmarkRows& rows, BenchmarkProgress progress = nullptr) {
    using Key = decltype(extractor(Person()));
    const DataOrder orders[] = {DataOrder::Random, DataOrder::Sorted, DataOrder::Reversed};

    // порядок данных - по значению того же атрибута, по которому строим индекс
    auto by_key = [&extractor](const Person& a, const Person& b) { return extractor(a) < extractor(b); };

    for (int s = 0; s < config.sizes.get_length(); s++) {
        int n = config.sizes.get(s);
        if (n <= 0)
            continue;

        for (DataOrder order : orders) {
            // одинаковый seed -> одинаковые люди для всех порядков, отличается только порядок
            PersonGenerator generator(config.seed);
            PersonSequencePtr data = generator.generate(n, order, by_key);

            std::mt19937 rng(config.seed + static_cast<unsigned>(n));
            auto queries = detail::make_queries<Key>(*data, extractor, config, rng);

            for (int k = 0; k <= INDEX_KIND_COUNT; k++) {
                BenchmarkRow row;
                if (k == INDEX_KIND_COUNT) {
                    row = detail::measure_linear<Key>(*data, extractor, queries, config.linear_queries);
                } else {
                    IndexKind kind = static_cast<IndexKind>(k);
                    if (is_slow_case(kind, order, n, config.slow_limit)) {
                        row.structure = to_string(kind);
                        row.skipped = true;
                    } else {
                        row = detail::measure_index<Key>(*data, extractor, kind, queries);
                    }
                }
                row.attribute = attribute_name;
                row.order = to_string(order);
                row.n = n;

                rows.append(row);
                if (progress != nullptr)
                    progress(row);
            }
        }
    }
}

// То же для атрибута из каталога Person
inline void run_attribute_benchmark(PersonAttribute attribute, const BenchmarkConfig& config,BenchmarkRows& rows, BenchmarkProgress progress = nullptr) {
    visit_attribute(attribute, config.age_year, [&](auto extractor) {
        run_attribute_benchmark(to_string(attribute), extractor, config, rows, progress);
    });
}
