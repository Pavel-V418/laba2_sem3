// Блок 7: таймер, прогон сравнения, выгрузка CSV

#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "benchmark/timer.h"
#include "benchmark/benchmark.h"
#include "benchmark/csv_export.h"

static BenchmarkConfig small_config() {
    BenchmarkConfig config;
    config.sizes = MutableArraySequence<int>();
    config.sizes.append(300);
    config.sizes.append(600);
    config.find_queries = 50;
    config.range_queries = 10;
    config.linear_queries = 5;
    config.slow_limit = 400;   // n = 600 -> медленные случаи пропускаются
    return config;
}

TEST(Timer, IsMonotonic) {
    Timer timer;
    double first = timer.elapsed_us();
    volatile long long sink = 0;
    for (int i = 0; i < 100000; i++)
        sink = sink + i;
    EXPECT_GE(timer.elapsed_us(), first);
    timer.restart();
    EXPECT_GE(timer.elapsed_ms(), 0.0);
}

TEST(Benchmark, SlowCases) {
    EXPECT_FALSE(is_slow_case(IndexKind::Sequence, DataOrder::Random, 100, 1000));
    EXPECT_TRUE(is_slow_case(IndexKind::Sequence, DataOrder::Random, 5000, 1000));
    EXPECT_TRUE(is_slow_case(IndexKind::BinaryTree, DataOrder::Sorted, 5000, 1000));
    EXPECT_FALSE(is_slow_case(IndexKind::BinaryTree, DataOrder::Random, 5000, 1000));
    EXPECT_FALSE(is_slow_case(IndexKind::AvlTree, DataOrder::Sorted, 5000, 1000));
    EXPECT_FALSE(is_slow_case(IndexKind::HashTable, DataOrder::Reversed, 5000, 1000));
}

TEST(Benchmark, RowsForEverySizeOrderAndStructure) {
    BenchmarkConfig config = small_config();
    BenchmarkRows rows;
    run_attribute_benchmark(PersonAttribute::Id, config, rows);

    // 2 размера x 3 порядка x (4 структуры + Linear)
    ASSERT_EQ(rows.get_length(), 2 * 3 * (INDEX_KIND_COUNT + 1));

    int skipped = 0;
    for (int i = 0; i < rows.get_length(); i++) {
        const BenchmarkRow& r = rows.get(i);
        EXPECT_EQ(r.attribute, "id");
        if (r.skipped) {
            skipped++;
            EXPECT_EQ(r.n, 600);
            continue;
        }
        EXPECT_GE(r.build_ms, 0.0);
        EXPECT_GE(r.find_us, 0.0);
        if (r.structure != "Linear") {
            EXPECT_EQ(r.distinct_keys, r.n);   // id уникален
        }
    }
    // n = 600 > slow_limit: Sequence на 3 порядках + BST на sorted и reversed
    EXPECT_EQ(skipped, 5);
}

// Все структуры на одних и тех же запросах должны найти одно и то же
TEST(Benchmark, AllStructuresFindTheSame) {
    BenchmarkConfig config = small_config();
    config.slow_limit = 10000;   // ничего не пропускаем
    BenchmarkRows rows;
    run_attribute_benchmark(PersonAttribute::BirthYearLastName, config, rows);

    for (int i = 0; i < rows.get_length(); i += INDEX_KIND_COUNT + 1) {
        const BenchmarkRow& first = rows.get(i);
        for (int k = 1; k < INDEX_KIND_COUNT; k++) {   // Linear делает меньше запросов - не сравниваем
            const BenchmarkRow& other = rows.get(i + k);
            EXPECT_EQ(other.n, first.n);
            EXPECT_EQ(other.order, first.order);
            EXPECT_EQ(other.find_hits, first.find_hits) << other.structure;
            EXPECT_EQ(other.range_hits, first.range_hits) << other.structure;
            EXPECT_EQ(other.distinct_keys, first.distinct_keys) << other.structure;
        }
    }
}

TEST(BenchmarkCsv, QuoteEscapesQuotes) {
    EXPECT_EQ(csv_quote("id"), "\"id\"");
    EXPECT_EQ(csv_quote("(a, b)"), "\"(a, b)\"");
    EXPECT_EQ(csv_quote("say \"hi\""), "\"say \"\"hi\"\"\"");
}

TEST(BenchmarkCsv, Format) {
    BenchmarkRows rows;
    BenchmarkRow ok;
    ok.attribute = "(first_name, last_name, birth_year)";
    ok.structure = "AVL";
    ok.order = "sorted";
    ok.n = 10;
    ok.build_ms = 1.5;
    rows.append(ok);

    BenchmarkRow skipped = ok;
    skipped.structure = "BST";
    skipped.skipped = true;
    rows.append(skipped);

    std::ostringstream out;
    write_benchmark_csv(out, rows);
    std::string text = out.str();

    EXPECT_EQ(text.find("attribute,structure,order,n,"), 0u);   // заголовок первой строкой
    EXPECT_NE(text.find("\"(first_name, last_name, birth_year)\",AVL,sorted,10,"), std::string::npos);
    EXPECT_NE(text.find(",BST,sorted,10,0,0,,,,,,1\n"), std::string::npos);   // пустые значения
}
