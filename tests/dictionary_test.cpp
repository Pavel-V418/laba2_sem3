// Блок 2: интерфейс IDictionary, итератор, исключения, наивный SequenceDictionary.
// Общие методы IDictionary (set, get_or_add, try_get, get_keys...) проверяем
// на SequenceDictionary - они реализованы в базовом классе один раз для всех.

#include <gtest/gtest.h>
#include <string>

#include "dictionary/sequence_dictionary.h"
#include "common/exception.h"

TEST(SequenceDictionary, EmptyDictionary) {
    SequenceDictionary<int, int> d;
    EXPECT_TRUE(d.is_empty());
    EXPECT_EQ(d.get_count(), 0);
    EXPECT_FALSE(d.contains_key(1));
    EXPECT_EQ(d.find(1), nullptr);
    EXPECT_FALSE(d.get_iterator()->has_next());
}

TEST(SequenceDictionary, AddGetRemove) {
    SequenceDictionary<std::string, int> d;
    d.add("one", 1);
    d.add("two", 2);
    d.add("three", 3);

    EXPECT_EQ(d.get_count(), 3);
    EXPECT_EQ(d.get("two"), 2);
    EXPECT_TRUE(d.contains_key("three"));

    d.remove("one");
    EXPECT_FALSE(d.contains_key("one"));
    EXPECT_EQ(d.get("three"), 3);   // удаление переставляет последний элемент - данные целы
    EXPECT_EQ(d.get_count(), 2);
}

TEST(SequenceDictionary, Exceptions) {
    SequenceDictionary<int, int> d;
    d.add(1, 10);
    EXPECT_THROW(d.add(1, 20), DuplicateKeyException);
    EXPECT_THROW(d.get(2), KeyNotFoundException);
    EXPECT_THROW(d.remove(2), KeyNotFoundException);
    EXPECT_EQ(d.get(1), 10);   // неудачные операции ничего не испортили
}

TEST(IDictionary, SetAddsOrReplaces) {
    SequenceDictionary<int, std::string> d;
    d.set(1, "a");
    d.set(1, "b");
    EXPECT_EQ(d.get_count(), 1);
    EXPECT_EQ(d.get(1), "b");
}

TEST(IDictionary, GetReturnsReference) {
    SequenceDictionary<int, int> d;
    d.add(1, 10);
    d.get(1) = 11;
    EXPECT_EQ(d.get(1), 11);
}

TEST(IDictionary, GetOrAdd) {
    SequenceDictionary<int, int> d;
    d.get_or_add(5) += 3;      // ключа не было - добавился 0
    d.get_or_add(5) += 3;
    EXPECT_EQ(d.get(5), 6);
    EXPECT_EQ(d.get_or_add(7, 100), 100);
}

TEST(IDictionary, TryGet) {
    SequenceDictionary<int, int> d;
    d.add(1, 10);
    EXPECT_TRUE(d.try_get(1).has_value());
    EXPECT_EQ(d.try_get(1).get_value(), 10);
    EXPECT_FALSE(d.try_get(2).has_value());
}

TEST(IDictionary, KeysAndValues) {
    SequenceDictionary<int, int> d;
    for (int i = 0; i < 10; i++)
        d.add(i, i * i);
    auto keys = d.get_keys();
    auto values = d.get_values();
    ASSERT_EQ(keys->get_length(), 10);
    ASSERT_EQ(values->get_length(), 10);
    for (int i = 0; i < 10; i++)
        EXPECT_EQ(values->get(i), keys->get(i) * keys->get(i));
}

TEST(IDictionary, ClearThenReuse) {
    SequenceDictionary<int, int> d;
    d.add(1, 1);
    d.clear();
    EXPECT_TRUE(d.is_empty());
    d.add(1, 2);
    EXPECT_EQ(d.get(1), 2);
}

TEST(IIterator, VisitsEveryItemOnce) {
    SequenceDictionary<int, int> d;
    for (int i = 0; i < 5; i++)
        d.add(i, -i);

    int visited = 0;
    for (auto it = d.get_iterator(); it->has_next(); it->next()) {
        EXPECT_EQ(it->get_current_item().value, -it->get_current_item().key);
        visited++;
    }
    EXPECT_EQ(visited, 5);
}

TEST(IIterator, AfterEnd) {
    SequenceDictionary<int, int> d;
    d.add(1, 1);
    auto it = d.get_iterator();
    EXPECT_FALSE(it->next());   // перешли за последний
    EXPECT_FALSE(it->has_next());
    EXPECT_FALSE(it->next());
    EXPECT_THROW(it->get_current_item(), IteratorOutOfRangeException);

    Pair<int, int> item;
    EXPECT_FALSE(it->try_get_current_item(item));
}
