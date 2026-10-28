// Временные smoke-тесты блоков 0, 3, 4 и 5.
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
#include "tree/binary_search_tree.h"
#include "tree/avl_tree.h"
#include "common/composite_key.h"
#include "common/composite_key_print.h"
#include "model/person_generator.h"
#include <climits>
#include <sstream>
#include <type_traits>

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


/*======================== Блок 4: BST и AVL ========================*/

// Большинство тестов шаблонные: одинаково гоняются на BinarySearchTree и AvlTree

template<class Tree>
static void test_tree_basic() {
    Tree t;
    assert(t.is_empty() && t.get_height() == 0 && t.check_invariants());
    assert(!t.get_iterator()->has_next());

    int keys[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    for (int k : keys)
        t.add(k, k * 10);
    assert(t.get_count() == 10 && t.check_invariants());
    assert(t.get(35) == 350 && t.contains_key(80) && !t.contains_key(55));
    assert(t.get_min_key() == 20 && t.get_max_key() == 80);

    t.get(35) = -1;                      // ссылка на значение
    assert(t.get(35) == -1);
    t.set(55, 550);                      // set: добавление
    t.set(55, 551);                      // set: замена
    assert(t.get(55) == 551 && t.get_count() == 11);

    // все три случая удаления
    t.remove(20);                        // лист
    t.remove(60);                        // один ребёнок (65)
    t.remove(30);                        // два ребёнка, преемник не прямой ребёнок
    t.remove(50);                        // корень с двумя детьми
    assert(t.check_invariants() && t.get_count() == 7);
    assert(!t.contains_key(20) && !t.contains_key(60) && !t.contains_key(30) && !t.contains_key(50));
    assert(t.get(65) == 650 && t.get(45) == 450);

    t.clear();
    assert(t.is_empty() && t.check_invariants());
    t.add(1, 1);                         // после clear дерево снова работает
    assert(t.get(1) == 1);
}

template<class Tree>
static void test_tree_exceptions() {
    Tree t;
    assert(throws([&] { t.get_min_key(); }));
    assert(throws([&] { t.remove(1); }));
    t.add(1, 10);
    try { t.add(1, 20); assert(false); } catch (const DuplicateKeyException&) {}
    try { t.get(2); assert(false); } catch (const KeyNotFoundException&) {}
    try { t.remove(2); assert(false); } catch (const KeyNotFoundException&) {}
    assert(t.get(1) == 10 && t.get_count() == 1 && t.check_invariants());
}

template<class Tree>
static void test_tree_iterator_sorted() {
    Tree t;
    std::mt19937 rng(7);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int i = 0; i < 3000; i++) {
        int k = dist(rng);
        if (!t.contains_key(k))
            t.add(k, -k);
    }

    int visited = 0;
    bool first = true;
    int previous = 0;
    for (auto it = t.get_iterator(); it->has_next(); it->next()) {
        const Pair<int, int>& item = it->get_current_item();
        assert(item.value == -item.key);
        assert(first || previous < item.key);     // строго по возрастанию
        previous = item.key;
        first = false;
        visited++;
    }
    assert(visited == t.get_count());

    // после конца: next() == false, get_current_item бросает
    auto it = t.get_iterator();
    while (it->next()) {}
    assert(!it->has_next() && !it->next());
    assert(throws([&] { it->get_current_item(); }));
    Pair<int, int> dummy;
    assert(!it->try_get_current_item(dummy));

    auto keys = t.get_keys();
    assert(keys->get_length() == t.get_count());
}

template<class Tree>
static void test_tree_range() {
    Tree t;
    for (int k = 0; k <= 100; k += 5)      // 0, 5, ..., 100
        t.add(k, k);

    auto r = t.find_range(12, 31);           // границы не являются ключами
    assert(r->get_length() == 4);
    assert(r->get(0).key == 15 && r->get(3).key == 30);

    assert(t.count_range(15, 30) == 4);      // границы включительно
    assert(t.count_range(0, 100) == 21);
    assert(t.count_range(-50, 1000) == 21);
    assert(t.count_range(20, 20) == 1);      // одна точка
    assert(t.count_range(21, 24) == 0);      // между ключами
    assert(t.count_range(101, 200) == 0);    // правее всех
    assert(t.count_range(-10, -1) == 0);     // левее всех
    assert(t.count_range(30, 10) == 0);      // from > to - пусто

    Tree empty;
    assert(empty.count_range(0, 10) == 0);

    // сверка с наивным перебором на случайных данных
    Tree big;
    SequenceDictionary<int, int> naive;
    std::mt19937 rng(99);
    std::uniform_int_distribution<int> dist(0, 5000);
    for (int i = 0; i < 1500; i++) {
        int k = dist(rng);
        if (!naive.contains_key(k)) {
            naive.add(k, i);
            big.add(k, i);
        }
    }
    for (int q = 0; q < 200; q++) {
        int a = dist(rng), b = dist(rng);
        if (a > b) { int tmp = a; a = b; b = tmp; }

        int expected = 0;
        for (auto it = naive.get_iterator(); it->has_next(); it->next())
            if (it->get_current_item().key >= a && it->get_current_item().key <= b)
                expected++;

        auto found = big.find_range(a, b);
        assert(found->get_length() == expected);
        for (int i = 0; i < found->get_length(); i++) {
            const Pair<int, int>& item = found->get(i);
            assert(item.key >= a && item.key <= b);
            assert(naive.get(item.key) == item.value);
            assert(i == 0 || found->get(i - 1).key < item.key);
        }
    }
}

// Сверка со SequenceDictionary на случайной последовательности операций + инварианты
template<class Tree>
static void test_tree_against_naive() {
    std::mt19937 rng(321);
    std::uniform_int_distribution<int> key_dist(0, 400);
    std::uniform_int_distribution<int> op_dist(0, 2);

    Tree tree;
    SequenceDictionary<int, int> naive;

    for (int step = 0; step < 20000; step++) {
        int key = key_dist(rng);
        switch (op_dist(rng)) {
            case 0:
                if (naive.contains_key(key)) {
                    assert(throws([&] { tree.add(key, step); }));
                } else {
                    tree.add(key, step);
                    naive.add(key, step);
                }
                break;
            case 1:
                if (naive.contains_key(key)) {
                    tree.remove(key);
                    naive.remove(key);
                } else {
                    assert(throws([&] { tree.remove(key); }));
                }
                break;
            default:
                assert(tree.contains_key(key) == naive.contains_key(key));
                if (naive.contains_key(key))
                    assert(tree.get(key) == naive.get(key));
        }
        assert(tree.get_count() == naive.get_count());
        if (step % 500 == 0)
            assert(tree.check_invariants());
    }
    assert(tree.check_invariants());
}

template<class Tree>
static void test_tree_copy() {
    Tree a;
    for (int i = 0; i < 100; i++)
        a.add((i * 37) % 101, i);
    Tree b = a;
    assert(b.check_invariants() && b.get_height() == a.get_height()); // форма сохранилась
    b.remove(0);
    b.get(37) = -5;
    assert(a.contains_key(0) && a.get(37) == 1);   // a не изменилась
    a = b;
    assert(!a.contains_key(0) && a.get(37) == -5 && a.check_invariants());
    a = a;                                          // самоприсваивание
    assert(a.get_count() == 99);
}

// Удаление перевешивает узлы, а не копирует данные: указатели на другие значения живы
template<class Tree>
static void test_tree_pointer_stability() {
    Tree t;
    for (int i = 1; i <= 15; i++)
        t.add(i, i * 100);
    int* p = t.find(9);
    t.remove(8);                          // 8 - узел с двумя детьми, 9 - его преемник
    t.remove(4);
    t.remove(12);
    assert(*p == 900);
    *p = 1;
    assert(t.get(9) == 1);
}

template<class Tree>
static void test_tree_string_keys() {
    Tree t;
    t.add("Ivanov", 1);
    t.add("Petrov", 2);
    t.add("Ivanova", 3);
    t.add("", 4);
    assert(t.get("Ivanova") == 3 && t.get("") == 4);
    assert(t.get_min_key() == "" && t.get_max_key() == "Petrov");
    assert(t.count_range("Ivanov", "Ivanovz") == 2);   // все фамилии с префиксом "Ivanov"
}

// Вырожденный случай: отсортированный вход
static void test_tree_degenerate() {
    // n = 20000: вставка в BST - O(n^2) ~ 2*10^8 сравнений. Главное - не упасть по стеку.
    const int n = 20000;
    BinarySearchTree<int, int> bst;
    AvlTree<int, int> avl;
    for (int i = 0; i < n; i++) {
        bst.add(i, i);
        avl.add(i, i);
    }
    assert(bst.get_height() == n);               // "палка"
    assert(avl.get_height() <= 16);              // log2(20000) ~ 14.3, предел AVL ~ 1.44 * log2(n)
    assert(bst.check_invariants() && avl.check_invariants());
    assert(bst.count_range(100, 199) == 100 && avl.count_range(100, 199) == 100);

    BinarySearchTree<int, int> bst_copy = bst;   // итеративное копирование палки
    assert(bst_copy.get_count() == n);
    // деструкторы палок тоже итеративные - выход из функции не должен упасть

    // AVL на 10^5 по убыванию - быстро и низко
    AvlTree<int, int> big;
    for (int i = 100000; i > 0; i--)
        big.add(i, i);
    assert(big.get_height() <= 24 && big.check_invariants());
    for (int i = 1; i <= 100000; i += 2)
        big.remove(i);
    assert(big.get_count() == 50000 && big.get_height() <= 23 && big.check_invariants());
}

static void test_tree_custom_less() {
    // обратный порядок: итератор идёт по убыванию, диапазон задаётся "в порядке дерева"
    AvlTree<int, int, KeyGreater<int>> t;
    for (int i = 1; i <= 10; i++)
        t.add(i, i);
    assert(t.get_min_key() == 10 && t.get_max_key() == 1);
    auto r = t.find_range(7, 3);
    assert(r->get_length() == 5 && r->get(0).key == 7 && r->get(4).key == 3);
    assert(t.check_invariants());
}

template<class Tree>
static void run_tree_tests() {
    test_tree_basic<Tree>();
    test_tree_exceptions<Tree>();
    test_tree_iterator_sorted<Tree>();
    test_tree_range<Tree>();
    test_tree_against_naive<Tree>();
    test_tree_copy<Tree>();
    test_tree_pointer_stability<Tree>();
}


/*======================== Блок 5: составные ключи ========================*/

static void test_composite_compare() {
    using Key = CompositeKey<int, std::string>;
    Key a(1990, "Ivanov"), b(1990, "Petrov"), c(1991, "Abramov");

    // решает первая различающаяся компонента
    assert(a < b && !(b < a));           // год равен -> сравниваем фамилии
    assert(b < c && !(c < b));           // год меньше -> фамилия не важна
    assert(a < c);                       // транзитивность
    assert(!(a < a));                    // строгий порядок: a < a ложно

    assert(a == Key(1990, "Ivanov"));
    assert(a != b);
    assert(c > a && a <= a && a >= a && b <= c);

    // ключ по умолчанию (нужен DynamicArray)
    Key d;
    assert(d.get<0>() == 0 && d.get<1>().empty());

    // три компоненты
    CompositeKey<std::string, std::string, int> x("Ivanov", "Ivan", 1990), y("Ivanov", "Ivan", 1991);
    assert(x < y && x != y);
    assert(CompositeKey<>() == CompositeKey<>() && !(CompositeKey<>() < CompositeKey<>()));
}

static void test_composite_get_and_make_key() {
    auto k = make_key(1990, "Ivanov", 'M');
    // строковый литерал хранится как std::string, а не как указатель
    static_assert(std::is_same<decltype(k), CompositeKey<int, std::string, char>>::value,
                  "make_key must convert string literals to std::string");
    static_assert(decltype(k)::SIZE == 3, "SIZE");

    assert(k.get<0>() == 1990 && k.get<1>() == "Ivanov" && k.get<2>() == 'M');
    k.get<1>() = "Petrov";               // неконстантный get - можно менять
    assert(k == make_key(1990, "Petrov", 'M'));

    std::ostringstream out;
    out << make_key(1990, "Ivanov", 7) << " " << CompositeKey<>();
    assert(out.str() == "(1990, Ivanov, 7) ()");
}

static void test_composite_hash() {
    Hasher<CompositeKey<int, int>> h2;
    assert(h2(make_key(1, 2)) == h2(make_key(1, 2)));    // равные ключи - равные хеши
    assert(h2(make_key(1, 2)) != h2(make_key(2, 1)));    // порядок важен

    Hasher<CompositeKey<std::string, std::string, int>> h3;
    assert(h3(make_key("Ivanov", "Ivan", 1990)) == h3(make_key("Ivanov", "Ivan", 1990)));
    assert(h3(make_key("Ivanov", "Ivan", 1990)) != h3(make_key("Ivan", "Ivanov", 1990)));

    // на сетке 100 x 100 почти нет совпадений хешей
    HashTable<CompositeKey<int, int>, int> table;
    for (int i = 0; i < 100; i++)
        for (int j = 0; j < 100; j++)
            table.add(make_key(i, j), i * 100 + j);
    assert(table.get_count() == 10000);
    assert(table.get(make_key(42, 17)) == 4217);
    assert((table.get_max_chain_length() <= HashTable<int, int>::MAX_CHAIN_LENGTH + 2));
}

// Хеш-таблица с ключом (фамилия, год) против наивного словаря
static void test_composite_in_hash_table() {
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

// Главный сценарий для блока 6: по году рождения много дубликатов, а словарь требует
// уникальных ключей. Ключ (год, id) уникален, а все люди одного года - это диапазон
// [(год, INT_MIN), (год, INT_MAX)]: лексикографический порядок кладёт их подряд.
template<class Tree>
static void test_composite_in_tree() {
    using Key = CompositeKey<int, int>;
    PersonGenerator generator(7);
    PersonSequencePtr people = generator.generate(3000);

    Tree index;
    for (int i = 0; i < people->get_length(); i++) {
        const Person& p = people->get(i);
        index.add(make_key(p.get_birth_year(), p.get_id()), p);
    }
    assert(index.get_count() == 3000);
    assert(index.check_invariants());

    // точный поиск по полному ключу
    const Person& someone = people->get(1234);
    assert(index.get(make_key(someone.get_birth_year(), someone.get_id())) == someone);

    // in-order обход: по году, внутри года - по id
    auto all = index.find_range(make_key(INT_MIN, INT_MIN), make_key(INT_MAX, INT_MAX));
    assert(all->get_length() == 3000);
    for (int i = 1; i < all->get_length(); i++)
        assert(all->get(i - 1).key < all->get(i).key);

    // поиск по префиксу (один год) и по диапазону лет - сверка с линейным перебором
    for (int year = 1940; year <= 2010; year += 7) {
        auto found = index.find_range(Key(year, INT_MIN), Key(year, INT_MAX));
        int expected = 0;
        for (int i = 0; i < people->get_length(); i++)
            if (people->get(i).get_birth_year() == year)
                expected++;
        assert(found->get_length() == expected);
        for (int i = 0; i < found->get_length(); i++) {
            assert(found->get(i).value.get_birth_year() == year);
            assert(i == 0 || found->get(i - 1).value.get_id() < found->get(i).value.get_id());
        }

        int to_year = year + 5;
        int expected_range = 0;
        for (int i = 0; i < people->get_length(); i++) {
            int y = people->get(i).get_birth_year();
            if (y >= year && y <= to_year)
                expected_range++;
        }
        assert(index.count_range(Key(year, INT_MIN), Key(to_year, INT_MAX)) == expected_range);
    }

    // удаление по составному ключу
    index.remove(make_key(someone.get_birth_year(), someone.get_id()));
    assert(!index.contains_key(make_key(someone.get_birth_year(), someone.get_id())));
    assert(index.check_invariants());
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

    run_tree_tests<BinarySearchTree<int, int>>();
    run_tree_tests<AvlTree<int, int>>();
    test_tree_string_keys<BinarySearchTree<std::string, int>>();
    test_tree_string_keys<AvlTree<std::string, int>>();
    test_tree_degenerate();
    test_tree_custom_less();
    std::cout << "Block 4 BST/AVL tests passed\n";

    test_composite_compare();
    test_composite_get_and_make_key();
    test_composite_hash();
    test_composite_in_hash_table();
    test_composite_in_tree<BinarySearchTree<CompositeKey<int, int>, Person>>();
    test_composite_in_tree<AvlTree<CompositeKey<int, int>, Person>>();
    std::cout << "Block 5 composite key tests passed\n";
    return 0;
}
