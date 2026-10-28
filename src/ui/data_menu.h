#pragma once

// Блок 9. Меню данных: генерация, ручной ввод, просмотр, файл.

#include <iostream>
#include <string>

#include "app_state.h"
#include "io/person_csv.h"

inline void GenerateData(AppState& state)
{
    std::cout << "Number of persons: ";
    int count = ReadIntInRange(0, 10000000);

    DataOrder order = ChooseOrder();

    std::cout << "Seed (empty for 42): ";
    int seed = ReadIntOrDefault(42);

    PersonGenerator generator(static_cast<unsigned>(seed));
    PersonSequencePtr generated;

    if (order == DataOrder::Random)
    {
        generated = generator.generate(count);
    }
    else
    {
        std::cout << "Sort by which attribute?";
        PersonAttribute attribute = ChooseAttribute();
        int year = 0;
        if (needs_year(attribute))
            year = 2026;   // порядок по возрасту на любой поздний год одинаковый

        visit_attribute(attribute, year, [&](auto extractor)
        {
            auto less = [&extractor](const Person& a, const Person& b) { return extractor(a) < extractor(b); };
            generated = generator.generate(count, order, less);
        });
    }

    state.data = *generated;
    std::cout << "Generated " << state.data.get_length() << " persons (" << to_string(order) << ")\n";
    AfterDataReplaced(state);
}

inline int NextId(const AppState& state)
{
    int result = 0;
    for (int i = 0; i < state.data.get_length(); i++)
        if (state.data.get(i).get_id() > result)
            result = state.data.get(i).get_id();
    return result + 1;
}

inline void AddPersonManually(AppState& state)
{
    std::cout << "Last name: ";
    std::string last = ReadNonEmptyLine();
    std::cout << "First name: ";
    std::string first = ReadNonEmptyLine();
    std::cout << "Middle name (empty if none): ";
    std::string middle = Trim(ReadLine());
    std::cout << "Birth year (" << Person::MIN_BIRTH_YEAR << ".." << Person::MAX_BIRTH_YEAR << "): ";
    int year = ReadIntInRange(Person::MIN_BIRTH_YEAR, Person::MAX_BIRTH_YEAR);

    // индекс по возрасту на год Y не может содержать человека, родившегося позже Y
    for (int i = 0; i < state.records.get_length(); i++)
    {
        const IndexRecord& record = state.records.get(i);
        if (needs_year(record.attribute) && record.year < year)
        {
            std::cout << "Cannot add: index '" << record.name << "' counts age on " << record.year
                      << ". Remove that index first.\n";
            return;
        }
    }

    Person person(NextId(state), last, first, middle, year);
    state.data.append(person);
    int position = state.data.get_length() - 1;
    state.indexes.notify_appended(position);   // индексы дополняются, а не строятся заново

    std::cout << "Added: ";
    PrintPerson(state, position);
}

inline void ShowData(const AppState& state)
{
    int length = state.data.get_length();
    if (length == 0)
    {
        std::cout << "Collection is empty\n";
        return;
    }

    std::cout << "Collection has " << length << " persons.\n";
    std::cout << "Start position (0.." << (length - 1) << "): ";
    int start = ReadIntInRange(0, length - 1);
    std::cout << "How many to show (empty for " << SHOW_LIMIT << "): ";
    int count = ReadIntOrDefault(SHOW_LIMIT);
    if (count < 0)
        count = 0;

    std::cout << "\n    pos  person\n";
    for (int i = start; i < length && i < start + count; i++)
        PrintPerson(state, i);
}

inline void LoadData(AppState& state)
{
    std::cout << "File path: ";
    std::string path = ReadNonEmptyLine();

    MutableArraySequence<Person> loaded;
    PersonCsvResult result;
    if (!load_persons_csv(path, loaded, result))
    {
        std::cout << "Cannot open file '" << path << "'\n";
        return;
    }

    state.data = loaded;
    std::cout << "Loaded " << result.loaded << " persons";
    if (result.skipped > 0)
        std::cout << ", skipped " << result.skipped << " bad line(s)";
    std::cout << "\n";
    AfterDataReplaced(state);
}

inline void SaveData(const AppState& state)
{
    std::cout << "File path (empty for persons.csv): ";
    std::string path = Trim(ReadLine());
    if (path.empty())
        path = "persons.csv";

    if (save_persons_csv(path, state.data))
        std::cout << "Saved " << state.data.get_length() << " persons to '" << path << "'\n";
    else
        std::cout << "Cannot write file '" << path << "'\n";
}

inline void DataMenu(AppState& state)
{
    int choice = -1;

    while (choice != 0)
    {
        std::cout << "\n===== DATA (" << state.data.get_length() << " persons) =====\n";
        std::cout << "1  Generate data\n";
        std::cout << "2  Add person manually\n";
        std::cout << "3  Show data\n";
        std::cout << "4  Load from file (CSV)\n";
        std::cout << "5  Save to file (CSV)\n";
        std::cout << "6  Clear data\n";
        std::cout << "0  Back\n";

        std::cout << "Choose operation: ";
        choice = ReadInt();

        if (choice == 1)
        {
            GenerateData(state);
        }

        else if (choice == 2)
        {
            AddPersonManually(state);
        }

        else if (choice == 3)
        {
            ShowData(state);
        }

        else if (choice == 4)
        {
            LoadData(state);
        }

        else if (choice == 5)
        {
            SaveData(state);
        }

        else if (choice == 6)
        {
            state.data = MutableArraySequence<Person>();
            AfterDataReplaced(state);
            std::cout << "Collection cleared\n";
        }
    }
}
