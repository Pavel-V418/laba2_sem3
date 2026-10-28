#pragma once

// Блок 9. Состояние программы и общие помощники меню.

#include <iomanip>
#include <iostream>
#include <string>

#include "console_input.h"
#include "model/person.h"
#include "model/person_print.h"
#include "model/person_attributes.h"
#include "model/person_generator.h"
#include "index/index_manager.h"
#include "benchmark/benchmark.h"
#include "LR2/mutableArraySequence.h"

// Что знает меню о каждом индексе (IndexManager хранит индексы без типа атрибута)
struct IndexRecord
{
    std::string name;
    PersonAttribute attribute = PersonAttribute::Id;
    int year = 0;   // для атрибутов с возрастом
};

struct AppState
{
    MutableArraySequence<Person> data;      // коллекция (индексы на неё ссылаются)
    IndexManager<Person> indexes;           // набор индексов над data
    MutableArraySequence<IndexRecord> records;
    BenchmarkConfig benchmark_config;
    BenchmarkRows last_benchmark;

    AppState()
        : data(), indexes(data), records(), benchmark_config(), last_benchmark() {}
};

static constexpr int SHOW_LIMIT = 20;   // сколько объектов показывать без вопроса "показать всё?"

/*============ Вывод ============*/

inline void PrintPerson(const AppState& state, int position)
{
    std::cout << std::setw(7) << position << "  " << state.data.get(position) << "\n";
}

// Печатает объекты по списку позиций: первые SHOW_LIMIT, остальное - по запросу
inline void PrintPositions(const AppState& state, const Sequence<int>& positions)
{
    int count = positions.get_length();
    int shown = count < SHOW_LIMIT ? count : SHOW_LIMIT;

    for (int i = 0; i < shown; i++)
        PrintPerson(state, positions.get(i));

    if (count > shown)
    {
        std::cout << "... and " << (count - shown) << " more. Show all? (y/n): ";
        if (ReadYesNo())
            for (int i = shown; i < count; i++)
                PrintPerson(state, positions.get(i));
    }
}

/*============ Выбор из списков ============*/

inline PersonAttribute ChooseAttribute()
{
    std::cout << "\nAttribute:\n";
    for (int i = 0; i < PERSON_ATTRIBUTE_COUNT; i++)
        std::cout << std::setw(2) << (i + 1) << "  " << to_string(person_attribute_at(i)) << "\n";
    std::cout << "Choice: ";
    return person_attribute_at(ReadIntInRange(1, PERSON_ATTRIBUTE_COUNT) - 1);
}

inline IndexKind ChooseIndexKind()
{
    std::cout << "\nStructure:\n";
    std::cout << "1  Sequence (naive linear dictionary)\n";
    std::cout << "2  Binary search tree\n";
    std::cout << "3  AVL tree\n";
    std::cout << "4  Hash table\n";
    std::cout << "Choice: ";
    return static_cast<IndexKind>(ReadIntInRange(1, INDEX_KIND_COUNT) - 1);
}

inline DataOrder ChooseOrder()
{
    std::cout << "\nData order:\n";
    std::cout << "1  Random\n";
    std::cout << "2  Sorted\n";
    std::cout << "3  Reversed\n";
    std::cout << "Choice: ";
    int choice = ReadIntInRange(1, 3);
    if (choice == 1)
        return DataOrder::Random;
    return choice == 2 ? DataOrder::Sorted : DataOrder::Reversed;
}

/*============ Проверки ============*/

inline int MaxBirthYear(const Sequence<Person>& data)
{
    int result = Person::MIN_BIRTH_YEAR;
    for (int i = 0; i < data.get_length(); i++)
        if (data.get(i).get_birth_year() > result)
            result = data.get(i).get_birth_year();
    return result;
}

inline int FindRecord(const AppState& state, const std::string& name)
{
    for (int i = 0; i < state.records.get_length(); i++)
        if (state.records.get(i).name == name)
            return i;
    return -1;
}

inline void RemoveIndex(AppState& state, int number)
{
    std::string name = state.records.get(number).name;
    state.indexes.remove(name);
    state.records.remove_at(number);
}

// Коллекция заменена целиком: индексы с возрастом на год раньше чьего-то рождения
// построить нельзя - удаляем их, остальные перестраиваем.
inline void AfterDataReplaced(AppState& state)
{
    int max_year = MaxBirthYear(state.data);
    for (int i = state.records.get_length() - 1; i >= 0; i--)
    {
        const IndexRecord& record = state.records.get(i);
        if (needs_year(record.attribute) && record.year < max_year)
        {
            std::cout << "Index '" << record.name << "' removed: age year " << record.year
                      << " is before birth year " << max_year << "\n";
            RemoveIndex(state, i);
        }
    }

    if (state.indexes.get_count() > 0)
    {
        Timer timer;
        state.indexes.rebuild_all();
        std::cout << "Rebuilt " << state.indexes.get_count() << " index(es) in "
                  << timer.elapsed_ms() << " ms\n";
    }
}

inline void PrintIndexList(const AppState& state)
{
    std::cout << "\n #  " << std::left << std::setw(48) << "name" << std::setw(11) << "structure"
              << std::right << std::setw(8) << "keys" << std::setw(9) << "items" << std::setw(8)
              << "height" << "\n";
    for (int i = 0; i < state.indexes.get_count(); i++)
    {
        const IIndex<Person>& index = state.indexes.get_index(i);
        std::cout << std::setw(2) << (i + 1) << "  " << std::left << std::setw(48) << index.get_name()
                  << std::setw(11) << to_string(index.get_kind()) << std::right << std::setw(8)
                  << index.get_key_count() << std::setw(9) << index.get_item_count() << std::setw(8)
                  << index.get_height() << "\n";
    }
}
