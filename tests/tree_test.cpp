// Блок 4: бинарное дерево поиска и AVL
#include <gtest/gtest.h>
#include <random>
#include <string>

#include "tree/binary_search_tree.h"
#include "tree/avl_tree.h"
#include "dictionary/sequence_dictionary.h"

template<class Tree>
class TreeTest : public ::testing::Test {};

using TreeTypes = ::testing::Types<BinarySearchTree<int, int>, AvlTree<int, int>>;
TYPED_TEST_SUITE(TreeTest, TreeTypes);

TYPED_TEST(TreeTest, Empty) {
    TypeParam t;
    EXPECT_TRUE(t.is_empty());
    EXPECT_EQ(t.get_height(), 0);
    EXPECT_TRUE(t.check_invariants());
    EXPECT_FALSE(t.get_iterator()->has_next());
    EXPECT_THROW(t.get_min_key(), KeyNotFoundException);
    EXPECT_THROW(t.get_max_key(), KeyNotFoundException);
}

TYPED_TEST(TreeTest, SingleElement) {
    TypeParam t;
    t.add(5, 50);
    EXPECT_EQ(t.get_height(), 1);
    EXPECT_EQ(t.get_min_key(), 5);
    EXPECT_EQ(t.get_max_key(), 5);
    t.remove(5);
    EXPECT_TRUE(t.is_empty());
    EXPECT_TRUE(t.check_invariants());
}

TYPED_TEST(TreeTest, AddGetSet) {
    TypeParam t;
    int keys[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    for (int k : keys)
        t.add(k, k * 10);

    EXPECT_EQ(t.get_count(), 10);
    EXPECT_TRUE(t.check_invariants());
    EXPECT_EQ(t.get(35), 350);
    EXPECT_TRUE(t.contains_key(80));
    EXPECT_FALSE(t.contains_key(55));
    EXPECT_EQ(t.get_min_key(), 20);
    EXPECT_EQ(t.get_max_key(), 80);

    t.get(35) = -1;
    EXPECT_EQ(t.get(35), -1);
    t.set(55, 550);
    t.set(55, 551);
    EXPECT_EQ(t.get(55), 551);
    EXPECT_EQ(t.get_count(), 11);
}

TYPED_TEST(TreeTest, RemoveAllCases) {
    TypeParam t;
    int keys[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    for (int k : keys)
        t.add(k, k * 10);

    t.remove(20);   // лист
    t.remove(60);   // один ребёнок
    t.remove(30);   // два ребёнка, преемник - не прямой ребёнок
    t.remove(50);   // корень с двумя детьми

    EXPECT_TRUE(t.check_invariants());
    EXPECT_EQ(t.get_count(), 6);
    EXPECT_FALSE(t.contains_key(20));
    EXPECT_FALSE(t.contains_key(50));
    EXPECT_EQ(t.get(65), 650);
    EXPECT_EQ(t.get(45), 450);
}

TYPED_TEST(TreeTest, Exceptions) {
    TypeParam t;
    EXPECT_THROW(t.remove(1), KeyNotFoundException);
    t.add(1, 10);
    EXPECT_THROW(t.add(1, 20), DuplicateKeyException);
    EXPECT_THROW(t.get(2), KeyNotFoundException);
    EXPECT_THROW(t.remove(2), KeyNotFoundException);
    EXPECT_EQ(t.get(1), 10);
    EXPECT_EQ(t.get_count(), 1);
    EXPECT_TRUE(t.check_invariants());
}

TYPED_TEST(TreeTest, ClearThenReuse) {
    TypeParam t;
    for (int i = 0; i < 100; i++)
        t.add(i, i);
    t.clear();
    EXPECT_TRUE(t.is_empty());
    EXPECT_TRUE(t.check_invariants());
    t.add(1, 1);
    EXPECT_EQ(t.get(1), 1);
}

/*============ Итератор (О-1.2) ============*/

TYPED_TEST(TreeTest, IteratorIsSorted) {
    TypeParam t;
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
        EXPECT_EQ(item.value, -item.key);
        if (!first) {
            EXPECT_LT(previous, item.key);
        }
        previous = item.key;
        first = false;
        visited++;
    }
    EXPECT_EQ(visited, t.get_count());
    EXPECT_EQ(t.get_keys()->get_length(), t.get_count());
}

TYPED_TEST(TreeTest, IteratorAfterEnd) {
    TypeParam t;
    t.add(1, 1);
    t.add(2, 2);
    auto it = t.get_iterator();
    while (it->next()) {}
    EXPECT_FALSE(it->has_next());
    EXPECT_FALSE(it->next());
    EXPECT_THROW(it->get_current_item(), IteratorOutOfRangeException);
}

/*============ Поиск по диапазону (И-5.1) ============*/

TYPED_TEST(TreeTest, RangeBorders) {
    TypeParam t;
    for (int k = 0; k <= 100; k += 5)   // 0, 5, ..., 100
        t.add(k, k);

    auto r = t.find_range(12, 31);       // границы не являются ключами
    ASSERT_EQ(r->get_length(), 4);
    EXPECT_EQ(r->get(0).key, 15);
    EXPECT_EQ(r->get(3).key, 30);

    EXPECT_EQ(t.count_range(15, 30), 4);   // границы включительно
    EXPECT_EQ(t.count_range(0, 100), 21);
    EXPECT_EQ(t.count_range(-50, 1000), 21);
    EXPECT_EQ(t.count_range(20, 20), 1);   // одна точка
    EXPECT_EQ(t.count_range(21, 24), 0);   // между ключами
    EXPECT_EQ(t.count_range(101, 200), 0); // правее всех
    EXPECT_EQ(t.count_range(-10, -1), 0);  // левее всех
    EXPECT_EQ(t.count_range(30, 10), 0);   // from > to - пусто

    TypeParam empty;
    EXPECT_EQ(empty.count_range(0, 10), 0);
}

TYPED_TEST(TreeTest, RangeMatchesNaive) {
    TypeParam tree;
    SequenceDictionary<int, int> naive;
    std::mt19937 rng(99);
    std::uniform_int_distribution<int> dist(0, 5000);
    for (int i = 0; i < 1500; i++) {
        int k = dist(rng);
        if (!naive.contains_key(k)) {
            naive.add(k, i);
            tree.add(k, i);
        }
    }

    for (int q = 0; q < 200; q++) {
        int a = dist(rng), b = dist(rng);
        if (a > b) {
            int tmp = a;
            a = b;
            b = tmp;
        }

        int expected = 0;
        for (auto it = naive.get_iterator(); it->has_next(); it->next())
            if (it->get_current_item().key >= a && it->get_current_item().key <= b)
                expected++;

        auto found = tree.find_range(a, b);
        ASSERT_EQ(found->get_length(), expected);
        for (int i = 0; i < found->get_length(); i++) {
            const Pair<int, int>& item = found->get(i);
            ASSERT_GE(item.key, a);
            ASSERT_LE(item.key, b);
            ASSERT_EQ(naive.get(item.key), item.value);
            if (i > 0) {
                ASSERT_LT(found->get(i - 1).key, item.key);
            }
        }
    }
}

/*============ Сверка, копирование, устойчивость указателей ============*/

TYPED_TEST(TreeTest, RandomOperationsMatchNaive) {
    std::mt19937 rng(321);
    std::uniform_int_distribution<int> key_dist(0, 400);
    std::uniform_int_distribution<int> op_dist(0, 2);

    TypeParam tree;
    SequenceDictionary<int, int> naive;

    for (int step = 0; step < 20000; step++) {
        int key = key_dist(rng);
        switch (op_dist(rng)) {
            case 0:
                if (naive.contains_key(key)) {
                    ASSERT_THROW(tree.add(key, step), DuplicateKeyException);
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
                    ASSERT_THROW(tree.remove(key), KeyNotFoundException);
                }
                break;
            default:
                ASSERT_EQ(tree.contains_key(key), naive.contains_key(key));
                if (naive.contains_key(key)) {
                    ASSERT_EQ(tree.get(key), naive.get(key));
                }
        }
        ASSERT_EQ(tree.get_count(), naive.get_count());
        if (step % 500 == 0) {
            ASSERT_TRUE(tree.check_invariants());
        }
    }
    EXPECT_TRUE(tree.check_invariants());
}

TYPED_TEST(TreeTest, CopyIsDeep) {
    TypeParam a;
    for (int i = 0; i < 100; i++)
        a.add((i * 37) % 101, i);

    TypeParam b = a;
    EXPECT_TRUE(b.check_invariants());
    EXPECT_EQ(b.get_height(), a.get_height());   // форма сохранилась
    b.remove(0);
    b.get(37) = -5;
    EXPECT_TRUE(a.contains_key(0));
    EXPECT_EQ(a.get(37), 1);

    a = b;
    EXPECT_FALSE(a.contains_key(0));
    EXPECT_EQ(a.get(37), -5);
    EXPECT_TRUE(a.check_invariants());

    TypeParam& self = a;
    a = self;   // самоприсваивание
    EXPECT_EQ(a.get_count(), 99);
}

// Удаление перевешивает узлы, а не копирует данные: указатели на другие значения живы
TYPED_TEST(TreeTest, PointersSurviveRemove) {
    TypeParam t;
    for (int i = 1; i <= 15; i++)
        t.add(i, i * 100);
    int* p = t.find(9);
    t.remove(8);   // 9 - преемник 8
    t.remove(4);
    t.remove(12);
    EXPECT_EQ(*p, 900);
    *p = 1;
    EXPECT_EQ(t.get(9), 1);
}

/*============ Нетипизированные тесты ============*/

TEST(Tree, StringKeys) {
    AvlTree<std::string, int> t;
    t.add("Ivanov", 1);
    t.add("Petrov", 2);
    t.add("Ivanova", 3);
    t.add("", 4);
    EXPECT_EQ(t.get("Ivanova"), 3);
    EXPECT_EQ(t.get(""), 4);
    EXPECT_EQ(t.get_min_key(), "");
    EXPECT_EQ(t.get_max_key(), "Petrov");
    EXPECT_EQ(t.count_range("Ivanov", "Ivanovz"), 2);   // все фамилии с префиксом "Ivanov"
}

// Отсортированный вход: BST вырождается в "палку", AVL остаётся низким
TEST(Tree, DegenerateInput) {
    const int n = 10000;
    BinarySearchTree<int, int> bst;
    AvlTree<int, int> avl;
    for (int i = 0; i < n; i++) {
        bst.add(i, i);
        avl.add(i, i);
    }
    EXPECT_EQ(bst.get_height(), n);
    EXPECT_LE(avl.get_height(), 15);   // ~1.44 * log2(n)
    EXPECT_TRUE(bst.check_invariants());
    EXPECT_TRUE(avl.check_invariants());
    EXPECT_EQ(bst.count_range(100, 199), 100);

    BinarySearchTree<int, int> copy = bst;   // итеративное копирование палки - без переполнения стека
    EXPECT_EQ(copy.get_count(), n);
}

TEST(Tree, AvlStaysBalancedAfterRemovals) {
    AvlTree<int, int> big;
    for (int i = 100000; i > 0; i--)
        big.add(i, i);
    EXPECT_LE(big.get_height(), 24);
    for (int i = 1; i <= 100000; i += 2)
        big.remove(i);
    EXPECT_EQ(big.get_count(), 50000);
    EXPECT_LE(big.get_height(), 23);
    EXPECT_TRUE(big.check_invariants());
}

TEST(Tree, CustomComparator) {
    // обратный порядок: итератор идёт по убыванию, диапазон задаётся "в порядке дерева"
    AvlTree<int, int, KeyGreater<int>> t;
    for (int i = 1; i <= 10; i++)
        t.add(i, i);
    EXPECT_EQ(t.get_min_key(), 10);
    EXPECT_EQ(t.get_max_key(), 1);

    auto r = t.find_range(7, 3);
    ASSERT_EQ(r->get_length(), 5);
    EXPECT_EQ(r->get(0).key, 7);
    EXPECT_EQ(r->get(4).key, 3);
    EXPECT_TRUE(t.check_invariants());
}
