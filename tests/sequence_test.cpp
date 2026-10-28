// Блок 0: Sequence из ЛР-1 - регрессии исправленных ошибок

#include <gtest/gtest.h>

#include "LR2/mutableArraySequence.h"
#include "LR2/immutableArraySequence.h"

TEST(Sequence, CopyAssignmentIsDeep) {
    MutableArraySequence<int> a;
    a.append(1)->append(2);
    MutableArraySequence<int> b;
    b.append(42);

    b = a;          // раньше: двойное delete[] -> падение
    a.set(100, 0);

    EXPECT_EQ(b.get(0), 1);   // b - независимая копия
    EXPECT_EQ(b.get_length(), 2);
}

TEST(Sequence, SetMutable) {
    MutableArraySequence<int> m;
    m.append(1)->append(2)->append(3);
    m.set(20, 1);
    EXPECT_EQ(m.get(1), 20);
}

TEST(Sequence, SetImmutableDoesNotChangeOriginal) {
    ImmutableArraySequence<int> empty;
    Sequence<int>* one = empty.append(1);
    Sequence<int>* changed = one->set(9, 0);

    EXPECT_EQ(one->get(0), 1);
    EXPECT_EQ(changed->get(0), 9);

    delete one;
    delete changed;
}

TEST(Sequence, TryGetOutOfRangeReturnsEmptyOption) {
    MutableArraySequence<int> s;
    s.append(1);
    EXPECT_TRUE(s.try_get(0).has_value());
    EXPECT_FALSE(s.try_get(1).has_value());   // раньше бросало исключение
    EXPECT_FALSE(s.try_get(-1).has_value());
}

TEST(Sequence, ConstOperations) {
    MutableArraySequence<int> a;
    a.append(1)->append(2);
    const Sequence<int>& ca = a;

    Sequence<int>* sum = ca + ca;   // раньше не компилировалось
    Sequence<int>* sub = ca.get_sub_sequence(1, 1);
    EXPECT_EQ(sum->get_length(), 4);
    EXPECT_EQ(sub->get(0), 2);

    delete sum;
    delete sub;
}

TEST(Sequence, AppendReferenceToOwnElement) {
    MutableArraySequence<int> a;
    a.append(7);
    for (int i = 0; i < 100; i++)
        a.append(a.get(0));   // ссылка внутрь массива + resize
    EXPECT_EQ(a.get_last(), 7);
    EXPECT_EQ(a.get_length(), 101);
}
