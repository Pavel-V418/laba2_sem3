// Блок 6: индексы и набор индексов (И-5, И-5.1, И-5.2).
// Главная проверка - сверка результата индекса с наивным линейным поиском
// на всех структурах (параметризованные тесты по IndexKind) и всех атрибутах Person.

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "index/index.h"
#include "index/index_manager.h"
#include "index/linear_search.h"
#include "model/person_generator.h"
#include "model/person_attributes.h"
#include "model/person_print.h"
#include "test_helpers.h"

static int get_year(const Person& p) {
    return p.get_birth_year();
}

class IndexKindTest : public ::testing::TestWithParam<IndexKind> {};

INSTANTIATE_TEST_SUITE_P(AllStructures, IndexKindTest,
                         ::testing::Values(IndexKind::Sequence, IndexKind::BinaryTree,
                                           IndexKind::AvlTree, IndexKind::HashTable),
                         [](const ::testing::TestParamInfo<IndexKind>& info) {
                             return std::string(to_string(info.param));
                         });

/*============ Граничные случаи ============*/

TEST_P(IndexKindTest, EmptyCollection) {
    MutableArraySequence<Person> empty;
    Index<Person, int> index("year", empty, get_year, GetParam());

    EXPECT_EQ(index.get_key_count(), 0);
    EXPECT_EQ(index.get_item_count(), 0);
    EXPECT_EQ(index.find(1990), nullptr);
    EXPECT_EQ(index.count(1990), 0);
    EXPECT_EQ(index.find_range(1900, 2100)->get_length(), 0);
}

TEST_P(IndexKindTest, SingleItem) {
    MutableArraySequence<Person> data;
    data.append(Person(1, "Ivanov", "Ivan", "", 1990));
    Index<Person, int> index("year", data, get_year, GetParam());

    ASSERT_NE(index.find(1990), nullptr);
    EXPECT_EQ(index.find(1990)->get(0), 0);   // позиция в коллекции
    EXPECT_FALSE(index.contains(1991));
    EXPECT_EQ(index.count_range(1990, 1990), 1);
    EXPECT_EQ(index.count_range(1991, 2000), 0);
}

TEST_P(IndexKindTest, DuplicatesKeepAllPositions) {
    MutableArraySequence<Person> data;
    data.append(Person(1, "Ivanov", "Ivan", "", 1990));
    data.append(Person(2, "Petrov", "Petr", "", 1985));
    data.append(Person(3, "Sidorov", "Ivan", "", 1990));
    data.append(Person(4, "Smirnov", "Oleg", "", 1990));
    Index<Person, int> index("year", data, get_year, GetParam());

    EXPECT_EQ(index.get_key_count(), 2);
    EXPECT_EQ(index.get_item_count(), 4);
    const PositionList* positions = index.find(1990);
    ASSERT_NE(positions, nullptr);
    ASSERT_EQ(positions->get_length(), 3);
    EXPECT_EQ(positions->get(0), 0);   // позиции по возрастанию
    EXPECT_EQ(positions->get(1), 2);
    EXPECT_EQ(positions->get(2), 3);
}

TEST_P(IndexKindTest, RangeReversedBordersIsEmpty) {
    PersonGenerator generator(1);
    PersonSequencePtr data = generator.generate(200);
    Index<Person, int> index("year", *data, get_year, GetParam());
    EXPECT_EQ(index.count_range(2000, 1950), 0);
}

TEST_P(IndexKindTest, CollectionIsNotChanged) {
    PersonGenerator generator(2);
    PersonSequencePtr data = generator.generate(300);
    MutableArraySequence<Person> copy = *data;

    Index<Person, int> index("year", *data, get_year, GetParam());
    index.find_range(1950, 1990);

    for (int i = 0; i < data->get_length(); i++)
        ASSERT_EQ(data->get(i), copy.get(i));   // п. 2.5 ТЗ: коллекцию не сортируем
}

/*============ Сверка с линейным поиском ============*/

TEST_P(IndexKindTest, ExactAndRangeMatchLinearSearch) {
    PersonGenerator generator(11);
    PersonSequencePtr data = generator.generate(2000);
    Index<Person, int> index("year", *data, get_year, GetParam());

    for (int year = 1935; year <= 2015; year++) {
        auto expected = linear_find(*data, get_year, year);
        const PositionList* found = index.find(year);
        if (expected->get_length() == 0) {
            EXPECT_EQ(found, nullptr) << "year " << year;
        } else {
            ASSERT_NE(found, nullptr) << "year " << year;
            EXPECT_TRUE(same_positions(*found, *expected)) << "year " << year;
        }
    }

    for (int from = 1935; from <= 2015; from += 9) {
        int to = from + 13;
        auto expected = linear_find_range(*data, get_year, from, to);
        auto found = index.find_range(from, to);
        EXPECT_TRUE(same_positions(*found, *expected)) << "[" << from << ", " << to << "]";
    }
}

// Все атрибуты каталога (в т.ч. составные из примеров ТЗ) x текущая структура
TEST_P(IndexKindTest, EveryPersonAttributeMatchesLinearSearch) {
    PersonGenerator generator(5);
    PersonSequencePtr data = generator.generate(800);

    for (int a = 0; a < PERSON_ATTRIBUTE_COUNT; a++) {
        PersonAttribute attribute = person_attribute_at(a);
        visit_attribute(attribute, 2019, [&](auto extractor) {
            using Key = decltype(extractor(Person()));
            Index<Person, Key> index(to_string(attribute), *data, extractor, GetParam());
            EXPECT_EQ(index.get_item_count(), data->get_length());

            for (int i = 0; i < data->get_length(); i += 37) {
                Key key = extractor(data->get(i));
                auto expected = linear_find(*data, extractor, key);
                const PositionList* found = index.find(key);
                ASSERT_NE(found, nullptr) << to_string(attribute);
                EXPECT_TRUE(same_positions(*found, *expected)) << to_string(attribute);
            }

            // диапазон между ключами двух объектов
            Key a_key = extractor(data->get(10));
            Key b_key = extractor(data->get(20));
            Key from = a_key < b_key ? a_key : b_key;
            Key to = a_key < b_key ? b_key : a_key;
            EXPECT_TRUE(same_positions(*index.find_range(from, to),
                                       *linear_find_range(*data, extractor, from, to)))
                << to_string(attribute);
        });
    }
}

/*============ Обновление ============*/

TEST_P(IndexKindTest, AddItemAndRebuild) {
    MutableArraySequence<Person> data;
    data.append(Person(1, "Ivanov", "Ivan", "", 1990));
    Index<Person, int> index("year", data, get_year, GetParam());

    data.append(Person(2, "Petrov", "Petr", "", 1990));
    index.add_item(1);
    EXPECT_EQ(index.count(1990), 2);
    EXPECT_THROW(index.add_item(5), std::out_of_range);

    data.set(Person(1, "Ivanov", "Ivan", "", 2000), 0);   // изменили объект "за спиной" индекса
    index.rebuild();
    EXPECT_EQ(index.count(1990), 1);
    EXPECT_EQ(index.count(2000), 1);
}

/*============ Свойства конкретных структур ============*/

TEST(Index, TreeRangeIsSortedByKey) {
    PersonGenerator generator(3);
    PersonSequencePtr data = generator.generate(1000);
    Index<Person, int> index("year", *data, get_year, IndexKind::AvlTree);

    auto found = index.find_range(1960, 1980);
    for (int i = 1; i < found->get_length(); i++)
        EXPECT_LE(data->get(found->get(i - 1)).get_birth_year(),
                  data->get(found->get(i)).get_birth_year());
}

TEST(Index, HeightOnlyForTrees) {
    PersonGenerator generator(3);
    PersonSequencePtr data = generator.generate(1000, DataOrder::Sorted, PersonByIdLess());
    auto by_id = [](const Person& p) { return p.get_id(); };

    Index<Person, int> bst("id", *data, by_id, IndexKind::BinaryTree);
    Index<Person, int> avl("id", *data, by_id, IndexKind::AvlTree);
    Index<Person, int> hash("id", *data, by_id, IndexKind::HashTable);

    EXPECT_EQ(bst.get_height(), 1000);   // отсортированный вход - вырождение
    EXPECT_LE(avl.get_height(), 15);
    EXPECT_EQ(hash.get_height(), 0);
}

TEST(Index, CustomComparator) {
    // "операция сравнения" как часть описания атрибута: годы по убыванию
    PersonGenerator generator(4);
    PersonSequencePtr data = generator.generate(500);
    Index<Person, int, KeyGreater<int>> index("year desc", *data, get_year, IndexKind::AvlTree);

    auto keys = index.get_keys();
    for (int i = 1; i < keys->get_length(); i++)
        EXPECT_GT(keys->get(i - 1), keys->get(i));

    // диапазон - "в порядке дерева": от большего к меньшему
    EXPECT_TRUE(same_positions(*index.find_range(1990, 1970),
                               *linear_find_range(*data, get_year, 1970, 1990)));
}

TEST(Index, CapturedLambda) {
    PersonGenerator generator(6);
    PersonSequencePtr data = generator.generate(300);
    int year = 2019;
    auto age = [year](const Person& p) { return p.get_age(year); };
    Index<Person, int> index("age", *data, age, IndexKind::HashTable);

    const Person& p = data->get(0);
    EXPECT_TRUE(same_positions(*index.find(p.get_age(2019)), *linear_find(*data, age, p.get_age(2019))));
}

/*============ IndexManager ============*/

TEST(IndexManager, AddGetRemove) {
    PersonGenerator generator(8);
    PersonSequencePtr data = generator.generate(500);
    IndexManager<Person> manager(*data);

    auto& by_year = manager.add_index<int>("year", get_year, IndexKind::AvlTree);
    manager.add_index<std::string>("last", [](const Person& p) { return p.get_last_name(); },
                                   IndexKind::HashTable);
    EXPECT_EQ(manager.get_count(), 2);
    EXPECT_TRUE(manager.contains("year"));

    EXPECT_EQ(&manager.get<int>("year"), &by_year);
    EXPECT_EQ(manager.get_index("last").get_kind(), IndexKind::HashTable);
    EXPECT_EQ(manager.get_index(0).get_name(), "year");

    manager.remove("year");
    EXPECT_FALSE(manager.contains("year"));
    EXPECT_EQ(manager.get_count(), 1);
}

TEST(IndexManager, Errors) {
    MutableArraySequence<Person> data;
    IndexManager<Person> manager(data);
    manager.add_index<int>("year", get_year, IndexKind::BinaryTree);

    EXPECT_THROW(manager.add_index<int>("year", get_year, IndexKind::AvlTree), DuplicateKeyException);
    EXPECT_THROW(manager.get_index("nope"), KeyNotFoundException);
    EXPECT_THROW(manager.remove("nope"), KeyNotFoundException);
    EXPECT_THROW(manager.get<std::string>("year"), std::invalid_argument);   // не тот тип ключа
    EXPECT_THROW(manager.get_index(5), std::out_of_range);
}

TEST(IndexManager, NotifyAndRebuild) {
    MutableArraySequence<Person> data;
    data.append(Person(1, "Ivanov", "Ivan", "", 1990));
    IndexManager<Person> manager(data);
    manager.add_index<int>("year", get_year, IndexKind::AvlTree);
    manager.add_index<int>("year hash", get_year, IndexKind::HashTable);

    data.append(Person(2, "Petrov", "Petr", "", 1990));
    manager.notify_appended(1);
    EXPECT_EQ(manager.get<int>("year").count(1990), 2);
    EXPECT_EQ(manager.get<int>("year hash").count(1990), 2);

    PersonGenerator generator(9);
    data = *generator.generate(100);   // коллекцию заменили целиком
    manager.rebuild_all();
    EXPECT_EQ(manager.get_index("year").get_item_count(), 100);
    EXPECT_EQ(manager.get_index("year hash").get_item_count(), 100);

    manager.clear();
    EXPECT_EQ(manager.get_count(), 0);
}
