#pragma once

// Блок 9. Меню индексов: построить, посмотреть список, удалить.

#include <iostream>
#include <string>

#include "app_state.h"

inline void BuildIndex(AppState& state)
{
    PersonAttribute attribute = ChooseAttribute();

    int year = 0;
    if (needs_year(attribute))
    {
        int min_year = MaxBirthYear(state.data);
        std::cout << "Count age on year (" << min_year << ".." << 2100 << ", empty for 2026): ";
        year = ReadIntOrDefault(2026);
        if (year < min_year || year > 2100)
        {
            std::cout << "Year must be in " << min_year << ".." << 2100 << "\n";
            return;
        }
    }

    IndexKind kind = ChooseIndexKind();

    std::string name = to_string(attribute);
    if (needs_year(attribute))
        name += " on " + std::to_string(year);
    name += std::string(" [") + to_string(kind) + "]";

    if (state.indexes.contains(name))
    {
        std::cout << "Index '" << name << "' already exists\n";
        return;
    }

    Timer timer;
    visit_attribute(attribute, year, [&](auto extractor)
    {
        using Key = decltype(extractor(Person()));
        state.indexes.add_index<Key>(name, extractor, kind);
    });
    double build_ms = timer.elapsed_ms();

    IndexRecord record;
    record.name = name;
    record.attribute = attribute;
    record.year = year;
    state.records.append(record);

    const IIndex<Person>& index = state.indexes.get_index(name);
    std::cout << "Built '" << name << "' in " << build_ms << " ms: " << index.get_key_count()
              << " distinct keys for " << index.get_item_count() << " persons";
    if (index.get_height() > 0)
        std::cout << ", tree height " << index.get_height();
    std::cout << "\n";
}

// Номер индекса из списка (1..count) или 0 - отмена
inline int ChooseIndexNumber(const AppState& state)
{
    PrintIndexList(state);
    std::cout << "Index number (0 - cancel): ";
    return ReadIntInRange(0, state.indexes.get_count());
}

inline void IndexMenu(AppState& state)
{
    int choice = -1;

    while (choice != 0)
    {
        std::cout << "\n===== INDEXES (" << state.indexes.get_count() << ") =====\n";
        std::cout << "1  Build index\n";
        std::cout << "2  List indexes\n";
        std::cout << "3  Remove index\n";
        std::cout << "4  Remove all indexes\n";
        std::cout << "0  Back\n";

        std::cout << "Choose operation: ";
        choice = ReadInt();

        if (choice == 1)
        {
            BuildIndex(state);
        }

        else if (choice == 2)
        {
            if (state.indexes.get_count() == 0)
                std::cout << "No indexes yet\n";
            else
                PrintIndexList(state);
        }

        else if (choice == 3)
        {
            if (state.indexes.get_count() == 0)
            {
                std::cout << "No indexes yet\n";
                continue;
            }
            int number = ChooseIndexNumber(state);
            if (number > 0)
            {
                std::cout << "Removed '" << state.records.get(number - 1).name << "'\n";
                RemoveIndex(state, number - 1);
            }
        }

        else if (choice == 4)
        {
            state.indexes.clear();
            state.records = MutableArraySequence<IndexRecord>();
            std::cout << "All indexes removed\n";
        }
    }
}
