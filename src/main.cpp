// ЛР-2, вариант И-5 "Индексирование данных". Консольный интерфейс (блок 9).

#include <exception>
#include <iostream>

#include "ui/app_state.h"
#include "ui/data_menu.h"
#include "ui/index_menu.h"
#include "ui/search_menu.h"
#include "ui/benchmark_menu.h"

void MainMenu(AppState& state)
{
    int choice = -1;

    while (choice != 0)
    {
        std::cout << "\n===== MAIN MENU =====\n";
        std::cout << "Persons: " << state.data.get_length() << " | Indexes: " << state.indexes.get_count()
                  << "\n";
        std::cout << "1. Data\n";
        std::cout << "2. Indexes\n";
        std::cout << "3. Search\n";
        std::cout << "4. Benchmark\n";
        std::cout << "0. Exit\n";

        std::cout << "Choice: ";
        choice = ReadInt();

        try
        {
            if (choice == 1)
                DataMenu(state);
            else if (choice == 2)
                IndexMenu(state);
            else if (choice == 3)
                SearchMenu(state);
            else if (choice == 4)
                BenchmarkMenu(state);
        }
        catch (const InputClosedException&)
        {
            throw;
        }
        catch (const std::exception& error)
        {
            // любая непредвиденная ошибка - сообщение, а не падение программы
            std::cout << "Error: " << error.what() << "\n";
        }
    }
}

int main()
{
    AppState state;

    // стартовые данные, чтобы сразу было что индексировать
    PersonGenerator generator(42);
    state.data = *generator.generate(1000);

    try
    {
        MainMenu(state);
    }
    catch (const InputClosedException&)
    {
        std::cout << "\nInput closed. Bye!\n";
    }

    return 0;
}
