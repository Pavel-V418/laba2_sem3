#pragma once

// Блок 9. Меню сравнения структур (бенчмарк из блока 7) и экспорт в CSV.

#include <iomanip>
#include <iostream>
#include <string>

#include "app_state.h"
#include "benchmark/csv_export.h"

inline void PrintBenchmarkHeader()
{
    std::cout << "\n" << std::left << std::setw(10) << "structure" << std::setw(10) << "order"
              << std::right << std::setw(8) << "n" << std::setw(8) << "keys" << std::setw(8)
              << "height" << std::setw(12) << "build ms" << std::setw(12) << "find us"
              << std::setw(12) << "range us" << "\n";
}

inline void PrintBenchmarkRow(const BenchmarkRow& row)
{
    std::cout << std::left << std::setw(10) << row.structure << std::setw(10) << row.order
              << std::right << std::setw(8) << row.n;
    if (row.skipped)
    {
        std::cout << "   skipped (O(n^2) build, n > slow limit)\n";
        return;
    }

    std::cout << std::setw(8) << row.distinct_keys << std::setw(8) << row.height << std::fixed
              << std::setprecision(3) << std::setw(12) << row.build_ms << std::setw(12) << row.find_us
              << std::setw(12) << row.range_us << "\n";
    std::cout.unsetf(std::ios::floatfield);
    std::cout << std::setprecision(6);
}

inline void PrintBenchmarkSettings(const BenchmarkConfig& config)
{
    std::cout << "sizes: ";
    for (int i = 0; i < config.sizes.get_length(); i++)
        std::cout << (i > 0 ? ", " : "") << config.sizes.get(i);
    std::cout << "\nexact queries: " << config.find_queries << ", range queries: " << config.range_queries
              << ", range width: " << config.range_percent << "% of n"
              << ", linear queries: " << config.linear_queries << "\nslow limit: " << config.slow_limit
              << ", seed: " << config.seed << ", age year: " << config.age_year << "\n";
}

// "1000 5000, 20000" -> [1000, 5000, 20000]; false - если есть ошибка
inline bool ParseSizes(const std::string& text, MutableArraySequence<int>& sizes)
{
    MutableArraySequence<int> result;
    std::string token;
    for (std::size_t i = 0; i <= text.size(); i++)
    {
        char c = i < text.size() ? text[i] : ' ';
        if (c == ' ' || c == ',' || c == ';' || c == '\t')
        {
            if (!token.empty())
            {
                int value;
                if (!TryParseInt(token, value) || value <= 0)
                    return false;
                result.append(value);
                token.clear();
            }
        }
        else
        {
            token += c;
        }
    }
    if (result.get_length() == 0)
        return false;
    sizes = result;
    return true;
}

inline void ChangeBenchmarkSettings(BenchmarkConfig& config)
{
    std::cout << "Sizes separated by spaces (empty - keep): ";
    std::string line = Trim(ReadLine());
    if (!line.empty() && !ParseSizes(line, config.sizes))
        std::cout << "Bad sizes, kept old ones\n";

    std::cout << "Exact queries (empty - " << config.find_queries << "): ";
    config.find_queries = ReadIntOrDefault(config.find_queries);
    std::cout << "Range queries (empty - " << config.range_queries << "): ";
    config.range_queries = ReadIntOrDefault(config.range_queries);
    std::cout << "Range width, % of n (empty - " << config.range_percent << "): ";
    config.range_percent = ReadIntOrDefault(config.range_percent);
    std::cout << "Linear search queries (empty - " << config.linear_queries << "): ";
    config.linear_queries = ReadIntOrDefault(config.linear_queries);
    std::cout << "Slow limit (empty - " << config.slow_limit << "): ";
    config.slow_limit = ReadIntOrDefault(config.slow_limit);
    std::cout << "Seed (empty - " << config.seed << "): ";
    config.seed = static_cast<unsigned>(ReadIntOrDefault(static_cast<int>(config.seed)));

    // отрицательные значения не имеют смысла
    if (config.find_queries < 0) config.find_queries = 0;
    if (config.range_queries < 0) config.range_queries = 0;
    if (config.linear_queries < 0) config.linear_queries = 0;
    if (config.range_percent < 0) config.range_percent = 0;
}

inline void RunBenchmark(AppState& state, PersonAttribute attribute)
{
    std::cout << "\nAttribute: " << to_string(attribute);
    PrintBenchmarkHeader();

    Timer timer;
    run_attribute_benchmark(attribute, state.benchmark_config, state.last_benchmark, PrintBenchmarkRow);
    std::cout << "Done in " << timer.elapsed_ms() / 1000 << " s\n";
}

inline void ExportBenchmark(const AppState& state)
{
    if (state.last_benchmark.get_length() == 0)
    {
        std::cout << "No results yet. Run the benchmark first.\n";
        return;
    }

    std::cout << "File path (empty for benchmark.csv): ";
    std::string path = Trim(ReadLine());
    if (path.empty())
        path = "benchmark.csv";

    if (save_benchmark_csv(path, state.last_benchmark))
    {
        std::cout << "Saved " << state.last_benchmark.get_length() << " rows to '" << path << "'\n";
        std::cout << "Plots: python scripts/plot_benchmark.py " << path << "\n";
    }
    else
    {
        std::cout << "Cannot write file '" << path << "'\n";
    }
}

inline void BenchmarkMenu(AppState& state)
{
    int choice = -1;

    while (choice != 0)
    {
        std::cout << "\n===== BENCHMARK =====\n";
        PrintBenchmarkSettings(state.benchmark_config);
        std::cout << "Results collected: " << state.last_benchmark.get_length() << " rows\n\n";
        std::cout << "1  Run for one attribute\n";
        std::cout << "2  Run for key attributes (id, birth_year, last_name, composite)\n";
        std::cout << "3  Change settings\n";
        std::cout << "4  Show collected results\n";
        std::cout << "5  Export results to CSV\n";
        std::cout << "6  Clear results\n";
        std::cout << "0  Back\n";

        std::cout << "Choose operation: ";
        choice = ReadInt();

        if (choice == 1)
        {
            RunBenchmark(state, ChooseAttribute());
        }

        else if (choice == 2)
        {
            RunBenchmark(state, PersonAttribute::Id);
            RunBenchmark(state, PersonAttribute::BirthYear);
            RunBenchmark(state, PersonAttribute::LastName);
            RunBenchmark(state, PersonAttribute::FirstLastBirthYear);
        }

        else if (choice == 3)
        {
            ChangeBenchmarkSettings(state.benchmark_config);
        }

        else if (choice == 4)
        {
            std::string attribute;
            for (int i = 0; i < state.last_benchmark.get_length(); i++)
            {
                const BenchmarkRow& row = state.last_benchmark.get(i);
                if (row.attribute != attribute)
                {
                    attribute = row.attribute;
                    std::cout << "\nAttribute: " << attribute;
                    PrintBenchmarkHeader();
                }
                PrintBenchmarkRow(row);
            }
        }

        else if (choice == 5)
        {
            ExportBenchmark(state);
        }

        else if (choice == 6)
        {
            state.last_benchmark = BenchmarkRows();
            std::cout << "Results cleared\n";
        }
    }
}
