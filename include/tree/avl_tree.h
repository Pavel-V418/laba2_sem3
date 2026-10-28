#pragma once

#include "binary_search_tree.h"
// из нового - балансировка
template<class TKey, class TValue, class Less = KeyLess<TKey>>
class AvlTree : public BinarySearchTree<TKey, TValue, Less> {

    using Base = BinarySearchTree<TKey, TValue, Less>; // базовый класс
    using Node = typename Base::Node;

public:
    explicit AvlTree(Less less = Less())
        : Base(less) {}

    // Высота хранится в корне: O(1)
    int get_height() const override {
        return height(this->root_);
    }

protected:
    // Хуки
    void after_insert(Node* node) override {
        rebalance_up(node->parent); // сам новый лист сбалансирован (высота 1) => подъём с родителя
    }

    void after_remove(Node* node_rebalance_from) override {
        rebalance_up(node_rebalance_from);
    }

    bool check_node(const Node* node) const override { // из check_invariants
        int expected = 1 + max_of(height(node->left), height(node->right));
        int balance = balance_factor(node);
        return node->height == expected && balance >= -1 && balance <= 1;
    }

private:
    // без привязки к объекту(дереву), т.к. его поля нам не нужны
    static int height(const Node* node) { // обращение только к полям Node
        return node != nullptr ? node->height : 0;
    }

    static int max_of(int a, int b) {
        return a > b ? a : b;
    }

    static void update_height(Node* node) {
        node->height = 1 + max_of(height(node->left), height(node->right));
    }

    // насколько левое поддерево выше правого (> 0 - перевес влево, < 0 - вправо)
    static int balance_factor(const Node* node) {
        return height(node->left) - height(node->right);
    }

    Node* rotate_left(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        if (y->left != nullptr)
            y->left->parent = x;

        this->replace_child(x->parent, x, y); // y встаёт на место x
        y->left = x;
        x->parent = y;

        update_height(x); // сначала x - он теперь ниже
        update_height(y);
        return y;
    }

    // зеркально rotate_left
    Node* rotate_right(Node* x) {
        Node* y = x->left;
        x->left = y->right;
        if (y->right != nullptr)
            y->right->parent = x;

        this->replace_child(x->parent, x, y);
        y->right = x;
        x->parent = y;

        update_height(x);
        update_height(y);
        return y;
    }

    // Балансирует один узел; возвращает узел, который теперь стоит на его месте
    Node* rebalance(Node* node) {
        update_height(node);
        int balance = balance_factor(node);

        if (balance > 1) {                               // перевес влево
            if (balance_factor(node->left) < 0)          // случай "лево-право"
                rotate_left(node->left);
            return rotate_right(node);
        }
        if (balance < -1) {                              // перевес вправо
            if (balance_factor(node->right) > 0)         // случай "право-лево"
                rotate_right(node->right);
            return rotate_left(node);
        }
        return node; // узел в порядке
    }

    // Подъём до корня: O(log n) узлов, на каждом O(1) работы
    void rebalance_up(Node* node) {
        while (node != nullptr) {
            node = rebalance(node);
            node = node->parent;
        }
    }
};
