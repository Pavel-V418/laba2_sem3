#pragma once

// Блок 9. Поиск через индекс: точный и по диапазону.
// Каждый поиск сразу проверяется линейным перебором (ручная + автоматическая проверка
// корректности из раздела 1 ТЗ) и сравнивается с ним по времени.

#include <iostream>
#include <string>

#include "app_state.h"
#include "common/composite_key.h"
#include "common/composite_key_print.h"
#include "index/linear_search.h"

/*============ Ввод ключа ============*/

inline void ReadKeyPart(int& value, const char* name)
{
    std::cout << "  " << name << ": ";
    value = ReadInt();
}

inline void ReadKeyPart(std::string& value, const char* name)
{
    std::cout << "  " << name << ": ";
    value = Trim(ReadLine());
}

// Составной ключ - по одной компоненте, рекурсивно (голова + хвост, как в composite_key.h)
inline void ReadKeyComponents(CompositeKey<>&, PersonAttribute, int) {}

template<class Head, class... Tail>
void ReadKeyComponents(CompositeKey<Head, Tail...>& key, PersonAttribute attribute, int component)
{
    ReadKeyPart(key.head, component_name(attribute, component));
    ReadKeyComponents(key.tail, attribute, component + 1);
}

template<class Key>
void ReadKey(Key& key, PersonAttribute attribute)
{
    ReadKeyPart(key, component_name(attribute, 0));
}

template<class... Ts>
void ReadKey(CompositeKey<Ts...>& key, PersonAttribute attribute)
{
    ReadKeyComponents(key, attribute, 0);
}

/*============ Поиск ============*/

// Вывод результата + сверка с перебором
inline void ReportSearch(const AppState& state, const Sequence<int>& found, double index_us,
                         const Sequence<int>& expected, double linear_us)
{
    std::cout << "\nFound " << found.get_length() << " person(s)\n";
    PrintPositions(state, found);

    std::cout << "\nIndex search:  " << index_us << " us\n";
    std::cout << "Linear search: " << linear_us << " us (" << expected.get_length() << " found)\n";
    std::cout << "Check vs linear search: "
              << (same_position_sets(found, expected) ? "OK" : "MISMATCH!") << "\n";
}

template<class Key, class F>
void ExactSearch(const AppState& state, const Index<Person, Key>& index, F extractor,
                 PersonAttribute attribute)
{
    Key key;
    std::cout << "Enter key:\n";
    ReadKey(key, attribute);

    Timer timer;
    const PositionList* positions = index.find(key);
    double index_us = timer.elapsed_us();

    PositionList found;
    if (positions != nullptr)
        found = *positions;

    timer.restart();
    auto expected = linear_find(state.data, extractor, key);
    double linear_us = timer.elapsed_us();

    ReportSearch(state, found, index_us, *expected, linear_us);
}

template<class Key, class F>
void RangeSearch(const AppState& state, const Index<Person, Key>& index, F extractor,
                 PersonAttribute attribute)
{
    Key from;
    Key to;
    std::cout << "From (inclusive):\n";
    ReadKey(from, attribute);
    std::cout << "To (inclusive):\n";
    ReadKey(to, attribute);

    if (!is_ordered(index.get_kind()))
        std::cout << "Note: " << to_string(index.get_kind())
                  << " has no key order - range search scans all keys, O(n)\n";

    Timer timer;
    PositionListPtr found = index.find_range(from, to);
    double index_us = timer.elapsed_us();

    timer.restart();
    auto expected = linear_find_range(state.data, extractor, from, to);
    double linear_us = timer.elapsed_us();

    ReportSearch(state, *found, index_us, *expected, linear_us);
}

// Различные значения атрибута и сколько объектов у каждого (у деревьев - по возрастанию)
template<class Key>
void ShowKeys(const Index<Person, Key>& index)
{
    auto keys = index.get_keys();
    int count = keys->get_length();
    std::cout << count << " distinct key(s)"
              << (is_ordered(index.get_kind()) ? " in ascending order" : " (hash order)") << ":\n";

    int shown = count < SHOW_LIMIT ? count : SHOW_LIMIT;
    for (int i = 0; i < shown; i++)
        std::cout << "  " << keys->get(i) << "  x" << index.count(keys->get(i)) << "\n";
    if (count > shown)
        std::cout << "  ... and " << (count - shown) << " more\n";
}

template<class Key, class F>
void IndexSearchMenu(const AppState& state, const Index<Person, Key>& index, F extractor,
                     PersonAttribute attribute)
{
    int choice = -1;

    while (choice != 0)
    {
        std::cout << "\n===== SEARCH: " << index.get_name() << " =====\n";
        std::cout << "1  Exact search\n";
        std::cout << "2  Range search\n";
        std::cout << "3  Show keys\n";
        std::cout << "0  Back\n";

        std::cout << "Choose operation: ";
        choice = ReadInt();

        if (choice == 1)
        {
            ExactSearch(state, index, extractor, attribute);
        }

        else if (choice == 2)
        {
            RangeSearch(state, index, extractor, attribute);
        }

        else if (choice == 3)
        {
            ShowKeys(index);
        }
    }
}

inline void SearchMenu(AppState& state)
{
    if (state.indexes.get_count() == 0)
    {
        std::cout << "No indexes. Build one in the Indexes menu first.\n";
        return;
    }

    std::cout << "\nChoose index for search:";
    PrintIndexList(state);
    std::cout << "Index number (0 - cancel): ";
    int number = ReadIntInRange(0, state.indexes.get_count());
    if (number == 0)
        return;

    IndexRecord record = state.records.get(number - 1);
    visit_attribute(record.attribute, record.year, [&](auto extractor)
    {
        using Key = decltype(extractor(Person()));
        const Index<Person, Key>& index = state.indexes.get<Key>(record.name);
        IndexSearchMenu(state, index, extractor, record.attribute);
    });
}
