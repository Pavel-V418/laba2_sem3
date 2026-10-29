// Блок 5: составные ключи

#include <gtest/gtest.h>
#include <climits>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>

#include "common/composite_key.h"
#include "common/composite_key_print.h"
#include "hash/hash_functions.h"
#include "dictionary/hash_table.h"
#include "dictionary/sequence_dictionary.h"
#include "tree/binary_search_tree.h"
#include "tree/avl_tree.h"
#include "model/person_generator.h"
#include "model/person_print.h"

/*============ Сравнение ============*/

TEST(CompositeKey, LexicographicOrder) {
    using Key = CompositeKey<int, std::string>;
    Key a(1990, "Ivanov"), b(1990, "Petrov"), c(1991, "Abramov");

    EXPECT_TRUE(a < b);    // год равен -> решает фамилия
    EXPECT_FALSE(b < a);
    EXPECT_TRUE(b < c);    // год меньше -> фамилия не важна
    EXPECT_FALSE(c < b);
    EXPECT_TRUE(a < c);    // транзитивность
    EXPECT_FALSE(a < a);   // строгий порядок
}

TEST(CompositeKey, EqualityAndDerivedOperators) {
    using Key = CompositeKey<int, std::string>;
    Key a(1990, "Ivanov"), b(1990, "Petrov"), c(1991, "Abramov");

    EXPECT_TRUE(a == Key(1990, "Ivanov"));
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(c > a);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a >= a);
    EXPECT_TRUE(b <= c);
}

TEST(CompositeKey, ThreeComponentsAndEmpty) {
    CompositeKey<std::string, std::string, int> x("Ivanov", "Ivan", 1990), y("Ivanov", "Ivan", 1991);
    EXPECT_TRUE(x < y);
    EXPECT_TRUE(x != y);

    EXPECT_TRUE(CompositeKey<>() == CompositeKey<>());
    EXPECT_FALSE(CompositeKey<>() < CompositeKey<>());
}

TEST(CompositeKey, DefaultConstructor) {
    CompositeKey<int, std::string> d;
    EXPECT_EQ(d.get<0>(), 0);
    EXPECT_TRUE(d.get<1>().empty());
}

/*============ get, make_key, вывод ============*/

TEST(CompositeKey, GetAndMakeKey) {
    auto k = make_key(1990, "Ivanov", 'M');
    // строковый литерал хранится как std::string, а не как указатель
    static_assert(std::is_same<decltype(k), CompositeKey<int, std::string, char>>::value, // decltype - получить тип выражения на этапе компиляции
                  "make_key must convert string literals to std::string");
    static_assert(decltype(k)::SIZE == 3, "SIZE");

    EXPECT_EQ(k.get<0>(), 1990);
    EXPECT_EQ(k.get<1>(), "Ivanov");
    EXPECT_EQ(k.get<2>(), 'M');

    k.get<1>() = "Petrov";
    EXPECT_TRUE(k == make_key(1990, "Petrov", 'M'));
}
// Вывод ключа (composite_key_print.h): компоненты в скобках через запятую, пустой ключ - ()
TEST(CompositeKey, Print) {
    std::ostringstream out;
    out << make_key(1990, "Ivanov", 7) << " " << CompositeKey<>();
    EXPECT_EQ(out.str(), "(1990, Ivanov, 7) ()");
}

/*============ Хеширование ============*/

TEST(CompositeKey, Hash) {
    Hasher<CompositeKey<int, int>> h2;
    EXPECT_EQ(h2(make_key(1, 2)), h2(make_key(1, 2)));   // равные ключи - равные хеши
    EXPECT_NE(h2(make_key(1, 2)), h2(make_key(2, 1)));   // порядок важен // !=

    Hasher<CompositeKey<std::string, std::string, int>> h3;
    EXPECT_EQ(h3(make_key("Ivanov", "Ivan", 1990)), h3(make_key("Ivanov", "Ivan", 1990)));
    EXPECT_NE(h3(make_key("Ivanov", "Ivan", 1990)), h3(make_key("Ivan", "Ivanov", 1990)));
}
// тест качества хеша
TEST(CompositeKey, GridHasFewCollisions) {
    HashTable<CompositeKey<int, int>, int> table;
    for (int i = 0; i < 100; i++)
        for (int j = 0; j < 100; j++)
            table.add(make_key(i, j), i * 100 + j);
    EXPECT_EQ(table.get_count(), 10000); // все 10 000 ключей добавились (дубликатов нет — значит, == различает ключи)
    EXPECT_EQ(table.get(make_key(42, 17)), 4217); // произвольный ключ находится с правильным значением
    EXPECT_LE(table.get_max_chain_length(), (HashTable<int, int>::MAX_CHAIN_LENGTH + 2)); // самая длинная цепочка не превышает предел таблицы
}

/*============ В словарях ============*/

TEST(CompositeKey, HashTableMatchesNaive) {
    using Key = CompositeKey<std::string, int>;
    const char* names[] = {"Ivanov", "Petrov", "Sidorov", "Smirnov", "Kuznetsov"};

    HashTable<Key, int> table;
    SequenceDictionary<Key, int> naive;
    std::mt19937 rng(555);
    std::uniform_int_distribution<int> name_dist(0, 4), year_dist(1980, 2000), op_dist(0, 2);

    for (int step = 0; step < 5000; step++) {
        Key key(names[name_dist(rng)], year_dist(rng));
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

// Ключ (год, id) уникален; все люди одного года - диапазон [(год, INT_MIN), (год, INT_MAX)]
template<class Tree>
class CompositeKeyTreeTest : public ::testing::Test {};

using CompositeTrees = ::testing::Types<BinarySearchTree<CompositeKey<int, int>, Person>,
                                        AvlTree<CompositeKey<int, int>, Person>>;
TYPED_TEST_SUITE(CompositeKeyTreeTest, CompositeTrees); // регистрирует тестовый набор

TYPED_TEST(CompositeKeyTreeTest, PrefixRangeByYear) {
    using Key = CompositeKey<int, int>;
    PersonGenerator generator(7);
    PersonSequencePtr people = generator.generate(3000);

    TypeParam tree;
    for (int i = 0; i < people->get_length(); i++) {
        const Person& p = people->get(i);
        tree.add(make_key(p.get_birth_year(), p.get_id()), p);
    }
    ASSERT_EQ(tree.get_count(), 3000);
    ASSERT_TRUE(tree.check_invariants());

    const Person& someone = people->get(1234);
    EXPECT_EQ(tree.get(make_key(someone.get_birth_year(), someone.get_id())), someone);

    for (int year = 1940; year <= 2010; year += 7) {
        auto found = tree.find_range(Key(year, INT_MIN), Key(year, INT_MAX));
        int expected = 0;
        for (int i = 0; i < people->get_length(); i++)
            if (people->get(i).get_birth_year() == year)
                expected++;
        ASSERT_EQ(found->get_length(), expected);
        for (int i = 0; i < found->get_length(); i++) {
            EXPECT_EQ(found->get(i).value.get_birth_year(), year);
            if (i > 0) {
                EXPECT_LT(found->get(i - 1).value.get_id(), found->get(i).value.get_id());
            }
        }
    }

    tree.remove(make_key(someone.get_birth_year(), someone.get_id()));
    EXPECT_FALSE(tree.contains_key(make_key(someone.get_birth_year(), someone.get_id())));
    EXPECT_TRUE(tree.check_invariants());
}
