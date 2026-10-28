// Блок 3: хеш-функции и хеш-таблица (М-5, М-5.1, О-1.4, пояснения 6-7 ТЗ)

#include <gtest/gtest.h>
#include <random>
#include <stdexcept>
#include <string>

#include "dictionary/hash_table.h"
#include "dictionary/sequence_dictionary.h"
#include "hash/hash_functions.h"

/*============ Хеш-функции ============*/

TEST(Hasher, EqualKeysEqualHashes) {
    EXPECT_EQ(Hasher<int>()(5), Hasher<int>()(5));
    EXPECT_EQ(Hasher<std::string>()("Ivanov"), Hasher<std::string>()("Ivanov"));
    EXPECT_EQ(Hasher<double>()(0.0), Hasher<double>()(-0.0));   // 0.0 == -0.0
}

TEST(Hasher, PairOrderMatters) {
    Hasher<Pair<int, int>> h;
    EXPECT_NE(h(Pair<int, int>(1, 2)), h(Pair<int, int>(2, 1)));
}

/*============ Основные операции ============*/

TEST(HashTable, Basic) {
    HashTable<int, std::string> t;
    EXPECT_TRUE(t.is_empty());
    EXPECT_EQ(t.get_capacity(), (HashTable<int, std::string>::DEFAULT_CAPACITY));

    t.add(1, "one");
    t.add(2, "two");
    t.add(-5, "minus five");
    EXPECT_EQ(t.get_count(), 3);
    EXPECT_EQ(t.get(1), "one");
    EXPECT_EQ(t.get(-5), "minus five");
    EXPECT_TRUE(t.contains_key(2));
    EXPECT_FALSE(t.contains_key(3));

    t.get(1) = "ONE";
    t.set(2, "TWO");
    t.set(3, "three");
    EXPECT_EQ(t.get(1), "ONE");
    EXPECT_EQ(t.get(2), "TWO");
    EXPECT_EQ(t.get(3), "three");

    t.remove(1);
    EXPECT_FALSE(t.contains_key(1));
    EXPECT_EQ(t.get_count(), 3);
}

TEST(HashTable, Exceptions) {
    HashTable<int, int> t;
    t.add(1, 10);
    EXPECT_THROW(t.add(1, 20), DuplicateKeyException);
    EXPECT_THROW(t.get(2), KeyNotFoundException);
    EXPECT_THROW(t.remove(2), KeyNotFoundException);
    EXPECT_EQ(t.get(1), 10);
}

TEST(HashTable, InvalidParameters) {
    // пояснение 7: p >= q > 1
    EXPECT_THROW((HashTable<int, int>(8, 1.5, 2.0)), std::invalid_argument);   // p < q
    EXPECT_THROW((HashTable<int, int>(8, 2.0, 1.0)), std::invalid_argument);   // q == 1
    EXPECT_THROW((HashTable<int, int>(0)), std::invalid_argument);             // c == 0
    EXPECT_NO_THROW((HashTable<int, int>(8, 2.0, 2.0)));                      // p == q допустимо
}

/*============ Перестройка (пояснение 7) ============*/

TEST(HashTable, GrowAndShrink) {
    const double p = 4.0, q = 2.0;
    HashTable<int, int> t(8, p, q);

    for (int i = 0; i < 1000; i++) {
        t.add(i, i * i);
        ASSERT_LT(t.get_count(), t.get_capacity());   // после add всегда n < c
    }
    int big_capacity = t.get_capacity();
    EXPECT_GE(big_capacity, 1000);

    for (int i = 0; i < 990; i++) {
        t.remove(i);
        // после remove: либо n > c/p, либо таблица уже минимальная
        ASSERT_TRUE(t.get_count() * p > t.get_capacity() || t.get_capacity() == 8);
    }
    EXPECT_LT(t.get_capacity(), big_capacity);
    EXPECT_EQ(t.get_count(), 10);
    for (int i = 990; i < 1000; i++)
        EXPECT_EQ(t.get(i), i * i);

    t.clear();
    EXPECT_TRUE(t.is_empty());
    EXPECT_EQ(t.get_capacity(), 8);
}

/*============ Коллизии (пояснение 6) ============*/

// Плохой хешер: у всех ключей один хеш -> всё в одной цепочке
struct ConstantHasher {
    HashCode operator()(int) const { return 42; }
};

TEST(HashTable, AllKeysCollide) {
    HashTable<int, int, ConstantHasher> bad;
    for (int i = 0; i < 200; i++)
        bad.add(i, i);
    for (int i = 0; i < 200; i++)
        EXPECT_EQ(bad.get(i), i);
    EXPECT_EQ(bad.get_max_chain_length(), 200);
    EXPECT_LT(bad.get_rebuild_count(), 40);   // нет бесконечных перестроек
    for (int i = 0; i < 200; i++)
        bad.remove(i);
    EXPECT_TRUE(bad.is_empty());
}

TEST(HashTable, ShortChainsOnGoodHasher) {
    HashTable<int, int> good;
    for (int i = 0; i < 100000; i++)
        good.add(i * 1024, i);   // кратные степени двойки - худший случай для "голого" % c
    EXPECT_LE(good.get_max_chain_length(), (HashTable<int, int>::MAX_CHAIN_LENGTH + 2));
}

TEST(HashTable, StringAndPairKeys) {
    HashTable<std::string, int> s;
    s.add("Ivanov", 1);
    s.add("Ivanova", 2);
    s.add("", 3);
    EXPECT_EQ(s.get("Ivanov"), 1);
    EXPECT_EQ(s.get("Ivanova"), 2);
    EXPECT_EQ(s.get(""), 3);

    HashTable<Pair<std::string, int>, int> composite;
    composite.add(Pair<std::string, int>("Petrov", 1990), 1);
    composite.add(Pair<std::string, int>("Petrov", 1991), 2);
    EXPECT_EQ(composite.get(Pair<std::string, int>("Petrov", 1991)), 2);
    EXPECT_FALSE(composite.contains_key(Pair<std::string, int>("Sidorov", 1991)));
}

/*============ Итератор (О-1.4) ============*/

TEST(HashTable, IteratorEmpty) {
    HashTable<int, int> empty;
    EXPECT_FALSE(empty.get_iterator()->has_next());
    EXPECT_THROW(empty.get_iterator()->get_current_item(), IteratorOutOfRangeException);
}

TEST(HashTable, IteratorVisitsEachKeyOnce) {
    HashTable<int, int> t;
    const int n = 500;
    for (int i = 0; i < n; i++)
        t.add(i * 7, i);

    DynamicArray<int> seen(n);
    for (int i = 0; i < n; i++)
        seen.set(0, i);

    int visited = 0;
    for (auto it = t.get_iterator(); it->has_next(); it->next()) {
        const Pair<int, int>& item = it->get_current_item();
        EXPECT_EQ(item.key, item.value * 7);
        seen.set(seen.get(item.value) + 1, item.value);
        visited++;
    }
    EXPECT_EQ(visited, n);
    for (int i = 0; i < n; i++)
        EXPECT_EQ(seen.get(i), 1);
}

/*============ Копирование и сверка с наивной реализацией ============*/

TEST(HashTable, CopyIsDeep) {
    HashTable<int, int> a;
    for (int i = 0; i < 50; i++)
        a.add(i, i);

    HashTable<int, int> b = a;
    b.remove(0);
    b.get(1) = 100;
    EXPECT_TRUE(a.contains_key(0));
    EXPECT_EQ(a.get(1), 1);

    a = b;
    EXPECT_FALSE(a.contains_key(0));
    EXPECT_EQ(a.get(1), 100);

    HashTable<int, int>& self = a;
    a = self;   // самоприсваивание
    EXPECT_EQ(a.get_count(), 49);
}

TEST(HashTable, RandomOperationsMatchNaive) {
    std::mt19937 rng(123);
    std::uniform_int_distribution<int> key_dist(0, 300);
    std::uniform_int_distribution<int> op_dist(0, 2);

    HashTable<int, int> table(4, 3.0, 1.5);
    SequenceDictionary<int, int> naive;

    for (int step = 0; step < 20000; step++) {
        int key = key_dist(rng);
        switch (op_dist(rng)) {
            case 0:
                if (naive.contains_key(key)) {
                    ASSERT_THROW(table.add(key, step), DuplicateKeyException);
                } else {
                    table.add(key, step);
                    naive.add(key, step);
                }
                break;
            case 1:
                if (naive.contains_key(key)) {
                    table.remove(key);
                    naive.remove(key);
                } else {
                    ASSERT_THROW(table.remove(key), KeyNotFoundException);
                }
                break;
            default:
                ASSERT_EQ(table.contains_key(key), naive.contains_key(key));
                if (naive.contains_key(key)) {
                    ASSERT_EQ(table.get(key), naive.get(key));
                }
        }
        ASSERT_EQ(table.get_count(), naive.get_count());
    }
}
