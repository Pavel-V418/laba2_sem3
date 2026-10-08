// Временные smoke-тесты блоков 0 и 3.
// В блоке 8 заменим на нормальный тестовый фреймворк.
#include <cassert>
#include <iostream>
#include <random>
#include <string>
#include <stdexcept>

#include "LR2/mutableArraySequence.h"
#include "LR2/immutableArraySequence.h"
#include "dictionary/hash_table.h"
#include "dictionary/sequence_dictionary.h"

/*======================== Блок 0: Sequence ========================*/

static void test_copy_assignment() {
    MutableArraySequence<int> a;
    a.append(1)->append(2);
    MutableArraySequence<int> b;
    b.append(42);
    b = a;                  // раньше: двойное delete[] -> падение
    a.set(100, 0);
    assert(b.get(0) == 1);  // b — независимая копия
    assert(b.get_length() == 2);
}

static void test_set() {
    MutableArraySequence<int> m;
    m.append(1)->append(2)->append(3);
    m.set(20, 1);
    assert(m.get(1) == 20);

    ImmutableArraySequence<int> im;
    Sequence<int>* im2 = im.append(1);
    Sequence<int>* im3 = im2->set(9, 0);
    assert(im2->get(0) == 1);   // исходная не изменилась
    assert(im3->get(0) == 9);
    delete im2;
    delete im3;
}

static void test_try_get() {
    MutableArraySequence<int> s;
    s.append(1);
    assert(s.try_get(0).has_value());
    assert(!s.try_get(1).has_value());  // раньше бросало исключение
    assert(!s.try_get(-1).has_value());
}

static void test_const_operations() {
    MutableArraySequence<int> a;
    a.append(1)->append(2);
    const Sequence<int>& ca = a;
    Sequence<int>* sum = ca + ca;       // раньше не компилировалось
    assert(sum->get_length() == 4);
    Sequence<int>* sub = ca.get_sub_sequence(1, 1);
    assert(sub->get(0) == 2);
    delete sum;
    delete sub;
}

static void test_self_append() {
    MutableArraySequence<int> a;
    a.append(7);
    for (int i = 0; i < 100; i++)
        a.append(a.get(0));             // ссылка внутрь массива + resize
    assert(a.get_last() == 7);
}

/*======================== Блок 3: HashTable ========================*/

template<class F>
static bool throws(F f) {
    try { f(); } catch (const std::exception&) { return true; }
    return false;
}

static void test_hash_basic() {
    HashTable<int, std::string> t;
    assert(t.is_empty());
    assert((t.get_capacity() == HashTable<int, std::string>::DEFAULT_CAPACITY));

    t.add(1, "one");
    t.add(2, "two");
    t.add(-5, "minus five");
    assert(t.get_count() == 3);
    assert(t.get(1) == "one");
    assert(t.get(-5) == "minus five");
    assert(t.contains_key(2));
    assert(!t.contains_key(3));
    assert(!t.try_get(3).has_value());
    assert(t.try_get(2).get_value() == "two");

    t.get(1) = "ONE";                   // get() отдаёт ссылку - можно менять
    assert(t.get(1) == "ONE");
    t.set(2, "TWO");                    // set: замена
    t.set(3, "three");                  // set: добавление
    assert(t.get(2) == "TWO" && t.get(3) == "three");

    t.remove(1);
    assert(!t.contains_key(1));
    assert(t.get_count() == 3);
}

static void test_hash_exceptions() {
    HashTable<int, int> t;
    t.add(1, 10);
    try { t.add(1, 20); assert(false); } catch (const DuplicateKeyException&) {}
    try { t.get(2); assert(false); } catch (const KeyNotFoundException&) {}
    try { t.remove(2); assert(false); } catch (const KeyNotFoundException&) {}
    assert(t.get(1) == 10);             // неудачные операции ничего не испортили

    // p >= q > 1
    assert(throws([] { HashTable<int, int> bad(8, 1.5, 2.0); }));   // p < q
    assert(throws([] { HashTable<int, int> bad(8, 2.0, 1.0); }));   // q == 1
    assert(throws([] { HashTable<int, int> bad(0); }));             // c == 0
    HashTable<int, int> ok(8, 2.0, 2.0);                            // p == q допустимо
}

static void test_hash_grow_and_shrink() {
    const double p = 4.0, q = 2.0;
    HashTable<int, int> t(8, p, q);

    for (int i = 0; i < 1000; i++) {
        t.add(i, i * i);
        assert(t.get_count() < t.get_capacity());       // после add всегда n < c
    }
    assert(t.get_capacity() >= 1000);
    int big_capacity = t.get_capacity();

    for (int i = 0; i < 1000; i++)
        assert(t.get(i) == i * i);

    for (int i = 0; i < 990; i++) {
        t.remove(i);
        // после remove либо n > c/p, либо таблица уже минимальная
        assert(t.get_count() * p > t.get_capacity() || t.get_capacity() == 8);
    }
    assert(t.get_capacity() < big_capacity);            // таблица сжалась
    assert(t.get_count() == 10);
    for (int i = 990; i < 1000; i++)
        assert(t.get(i) == i * i);

    t.clear();
    assert(t.is_empty() && t.get_capacity() == 8);
}

static void test_hash_iterator() {
    HashTable<int, int> empty;
    assert(!empty.get_iterator()->has_next());
    assert(throws([&] { empty.get_iterator()->get_current_item(); }));

    HashTable<int, int> t;
    const int n = 500;
    for (int i = 0; i < n; i++)
        t.add(i * 7, i);

    // каждый ключ встречается ровно один раз
    DynamicArray<int> seen(n);
    for (int i = 0; i < n; i++)
        seen.set(0, i);

    int visited = 0;
    for (auto it = t.get_iterator(); it->has_next(); it->next()) {
        const Pair<int, int>& item = it->get_current_item();
        assert(item.key == item.value * 7);
        seen.set(seen.get(item.value) + 1, item.value);
        visited++;
    }
    assert(visited == n);
    for (int i = 0; i < n; i++)
        assert(seen.get(i) == 1);

    assert(t.get_keys()->get_length() == n);
    assert(t.get_values()->get_length() == n);
}

static void test_hash_string_and_pair_keys() {
    HashTable<std::string, int> s;
    s.add("Ivanov", 1);
    s.add("Ivanova", 2);
    s.add("", 3);
    assert(s.get("Ivanov") == 1 && s.get("Ivanova") == 2 && s.get("") == 3);

    // кортеж (фамилия, год) - будущий составной индекс
    HashTable<Pair<std::string, int>, int> composite;
    composite.add(Pair<std::string, int>("Petrov", 1990), 1);
    composite.add(Pair<std::string, int>("Petrov", 1991), 2);
    composite.add(Pair<std::string, int>("Sidorov", 1990), 3);
    assert(composite.get(Pair<std::string, int>("Petrov", 1991)) == 2);
    assert(!composite.contains_key(Pair<std::string, int>("Sidorov", 1991)));

    // порядок компонент важен
    Hasher<Pair<int, int>> h;
    assert(h(Pair<int, int>(1, 2)) != h(Pair<int, int>(2, 1)));
    assert(Hasher<double>()(0.0) == Hasher<double>()(-0.0));
}

// Плохой хешер: у всех ключей хеш совпадает -> всё в одной цепочке.
// Таблица обязана оставаться корректной (пусть и медленной) и не уходить в бесконечные перестройки.
struct ConstantHasher {
    HashCode operator()(int) const { return 42; }
};

static void test_hash_collisions() {
    HashTable<int, int, ConstantHasher> bad;
    for (int i = 0; i < 200; i++)
        bad.add(i, i);
    for (int i = 0; i < 200; i++)
        assert(bad.get(i) == i);
    assert(bad.get_max_chain_length() == 200);
    // перестроек не больше, чем изменений размера * 2 (смена seed - максимум одна на размер)
    assert(bad.get_rebuild_count() < 40);
    for (int i = 0; i < 200; i++)
        bad.remove(i);
    assert(bad.is_empty());

    // на хорошем хешере цепочки короткие
    HashTable<int, int> good;
    for (int i = 0; i < 100000; i++)
        good.add(i * 1024, i);          // кратные степени двойки - худший случай для "голого" % c
    assert((good.get_max_chain_length() <= HashTable<int, int>::MAX_CHAIN_LENGTH + 2));
}

// Сверка с наивной реализацией на случайной последовательности операций
static void test_hash_against_naive() {
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
                    assert(throws([&] { table.add(key, step); }));
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
                    assert(throws([&] { table.remove(key); }));
                }
                break;
            default:
                assert(table.contains_key(key) == naive.contains_key(key));
                if (naive.contains_key(key))
                    assert(table.get(key) == naive.get(key));
        }
        assert(table.get_count() == naive.get_count());
    }
}

static void test_hash_copy() {
    HashTable<int, int> a;
    for (int i = 0; i < 50; i++)
        a.add(i, i);
    HashTable<int, int> b = a;
    b.remove(0);
    b.get(1) = 100;
    assert(a.contains_key(0) && a.get(1) == 1);   // a не изменилась
    a = b;
    assert(!a.contains_key(0) && a.get(1) == 100);
    a = a;                                         // самоприсваивание
    assert(a.get_count() == 49);
}

int main() {
    test_copy_assignment();
    test_set();
    test_try_get();
    test_const_operations();
    test_self_append();
    std::cout << "Block 0 smoke tests passed\n";

    test_hash_basic();
    test_hash_exceptions();
    test_hash_grow_and_shrink();
    test_hash_iterator();
    test_hash_string_and_pair_keys();
    test_hash_collisions();
    test_hash_against_naive();
    test_hash_copy();
    std::cout << "Block 3 hash table tests passed\n";
    return 0;
}
