// Блок 1: Person, генератор данных и собственные алгоритмы над массивами

#include <gtest/gtest.h>
#include <random>
#include <stdexcept>

#include "model/person.h"
#include "model/person_print.h"
#include "model/person_generator.h"
#include "common/array_algorithms.h"
#include "common/pair.h"

/*============ Person ============*/

TEST(Person, Getters) {
    Person p(7, "Ivanov", "Ivan", "Petrovich", 1990);
    EXPECT_EQ(p.get_id(), 7);
    EXPECT_EQ(p.get_last_name(), "Ivanov");
    EXPECT_EQ(p.get_first_name(), "Ivan");
    EXPECT_EQ(p.get_middle_name(), "Petrovich");
    EXPECT_EQ(p.get_birth_year(), 1990);
    EXPECT_EQ(p.get_full_name(), "Ivanov Ivan Petrovich");
    EXPECT_EQ(p.get_fio(), "Ivanov I.P.");
}

TEST(Person, EmptyMiddleName) {
    Person p(1, "Petrova", "Anna", "", 1985);
    EXPECT_EQ(p.get_full_name(), "Petrova Anna");
    EXPECT_EQ(p.get_fio(), "Petrova A.");
}

TEST(Person, Age) {
    Person p(1, "Ivanov", "Ivan", "", 1990);
    EXPECT_EQ(p.get_age(2019), 29);
    EXPECT_EQ(p.get_age(1990), 0);
    EXPECT_THROW(p.get_age(1989), std::invalid_argument);
}

TEST(Person, InvalidData) {
    EXPECT_THROW(Person(1, "", "Ivan", "", 1990), std::invalid_argument);
    EXPECT_THROW(Person(1, "Ivanov", "", "", 1990), std::invalid_argument);
    EXPECT_THROW(Person(1, "Ivanov", "Ivan", "", Person::MIN_BIRTH_YEAR - 1), std::invalid_argument);
    EXPECT_THROW(Person(1, "Ivanov", "Ivan", "", Person::MAX_BIRTH_YEAR + 1), std::invalid_argument);
}

TEST(Person, Equality) {
    Person a(1, "Ivanov", "Ivan", "", 1990);
    Person b(1, "Ivanov", "Ivan", "", 1990);
    Person c(2, "Ivanov", "Ivan", "", 1990);
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
}

/*============ Генератор ============*/

TEST(PersonGenerator, SizeAndUniqueIds) {
    PersonGenerator generator(1);
    PersonSequencePtr people = generator.generate(500);
    ASSERT_EQ(people->get_length(), 500);

    // id - перестановка 1..500
    DynamicArray<int> seen(501);
    for (int i = 0; i <= 500; i++)
        seen.set(0, i);
    for (int i = 0; i < 500; i++) {
        int id = people->get(i).get_id();
        ASSERT_GE(id, 1);
        ASSERT_LE(id, 500);
        seen.set(seen.get(id) + 1, id);
    }
    for (int id = 1; id <= 500; id++)
        EXPECT_EQ(seen.get(id), 1);
}

TEST(PersonGenerator, SameSeedSameData) {
    PersonGenerator a(42), b(42);
    PersonSequencePtr x = a.generate(100);
    PersonSequencePtr y = b.generate(100);
    for (int i = 0; i < 100; i++)
        EXPECT_EQ(x->get(i), y->get(i));
}

TEST(PersonGenerator, Orders) {
    PersonGenerator generator(3);
    PersonSequencePtr sorted = generator.generate(300, DataOrder::Sorted, PersonByBirthYearLess());
    EXPECT_TRUE(is_sorted(*sorted, PersonByBirthYearLess()));

    PersonSequencePtr reversed = generator.generate(300, DataOrder::Reversed, PersonByIdLess());
    for (int i = 1; i < reversed->get_length(); i++)
        EXPECT_GT(reversed->get(i - 1).get_id(), reversed->get(i).get_id());
}

TEST(PersonGenerator, EdgeSizes) {
    PersonGenerator generator;
    EXPECT_EQ(generator.generate(0)->get_length(), 0);
    EXPECT_EQ(generator.generate(1)->get_length(), 1);
    EXPECT_THROW(generator.generate(-1), std::invalid_argument);
}

/*============ array_algorithms ============*/

TEST(ArrayAlgorithms, MergeSortIsStable) {
    // сортируем по ключу (первая цифра), второе число - исходный порядок
    DynamicArray<Pair<int, int>> arr(8);
    int keys[] = {3, 1, 3, 2, 1, 3, 2, 1};
    for (int i = 0; i < 8; i++)
        arr.set(Pair<int, int>(keys[i], i), i);

    merge_sort(arr, [](const Pair<int, int>& a, const Pair<int, int>& b) { return a.key < b.key; });

    for (int i = 1; i < 8; i++) {
        EXPECT_LE(arr.get(i - 1).key, arr.get(i).key);
        if (arr.get(i - 1).key == arr.get(i).key) {
            EXPECT_LT(arr.get(i - 1).value, arr.get(i).value);   // равные не переставлены
        }
    }
}

TEST(ArrayAlgorithms, MergeSortEdgeCases) {
    DynamicArray<int> empty(0);
    merge_sort(empty, [](int a, int b) { return a < b; });
    EXPECT_EQ(empty.get_size(), 0);

    DynamicArray<int> one(1);
    one.set(5, 0);
    merge_sort(one, [](int a, int b) { return a < b; });
    EXPECT_EQ(one.get(0), 5);
}

TEST(ArrayAlgorithms, ReverseAndShuffle) {
    DynamicArray<int> arr(100);
    for (int i = 0; i < 100; i++)
        arr.set(i, i);

    reverse(arr);
    for (int i = 0; i < 100; i++)
        EXPECT_EQ(arr.get(i), 99 - i);

    std::mt19937 rng(5);
    shuffle(arr, rng);
    merge_sort(arr, [](int a, int b) { return a < b; });   // перемешивание - перестановка
    for (int i = 0; i < 100; i++)
        EXPECT_EQ(arr.get(i), i);
}

TEST(ArrayAlgorithms, SortedCopyKeepsSource) {
    MutableArraySequence<int> source;
    source.append(3)->append(1)->append(2);
    auto copy = sorted_copy(source, [](int a, int b) { return a < b; });

    EXPECT_EQ(source.get(0), 3);   // исходная не изменилась
    EXPECT_TRUE(is_sorted(*copy, [](int a, int b) { return a < b; }));
    EXPECT_FALSE(is_sorted(source, [](int a, int b) { return a < b; }));
}
