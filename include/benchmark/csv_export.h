#pragma once

// Блок 7. Выгрузка результатов в CSV - "выгрузка необходимых данных в открытых форматах
// (например, csv)" (раздел 1 ТЗ). Графики строит scripts/plot_benchmark.py.

#include <fstream>
#include <ostream>
#include <string>
#include "benchmark.h"

// Поле CSV: в кавычках, внутренние кавычки удваиваются (в имени атрибута есть запятые)
inline std::string csv_quote(const std::string& value) {
    std::string result = "\"";
    for (std::size_t i = 0; i < value.size(); i++) {
        if (value[i] == '"')
            result += '"';
        result += value[i];
    }
    result += '"';
    return result;
}

inline void write_benchmark_csv(std::ostream& out, const BenchmarkRows& rows) {
    out << "attribute,structure,order,n,distinct_keys,height,build_ms,find_us,range_us,"
           "find_hits,range_hits,skipped\n";
    for (int i = 0; i < rows.get_length(); i++) {
        const BenchmarkRow& r = rows.get(i);
        out << csv_quote(r.attribute) << ',' << r.structure << ',' << r.order << ',' << r.n << ','
            << r.distinct_keys << ',' << r.height << ',';
        if (r.skipped)
            out << ",,,,,1\n";   // пустые значения - чтобы на графике не было ложных нулей
        else
            out << r.build_ms << ',' << r.find_us << ',' << r.range_us << ','
                << r.find_hits << ',' << r.range_hits << ",0\n";
    }
}

// true - файл записан
inline bool save_benchmark_csv(const std::string& path, const BenchmarkRows& rows) {
    std::ofstream file(path);
    if (!file)
        return false;
    write_benchmark_csv(file, rows);
    return static_cast<bool>(file);
}
