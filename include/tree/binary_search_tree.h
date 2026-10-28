#pragma once

#include <memory>
#include "../dictionary/i_dictionary.h"
#include "../common/comparators.h"
#include "../LR2/mutableArraySequence.h"

template<class TKey, class TValue, class Less = KeyLess<TKey>>
class BinarySearchTree : public IDictionary<TKey, TValue> {

public:
    using Item = typename IDictionary<TKey, TValue>::Item; // Pair
    using IteratorPtr = typename IDictionary<TKey, TValue>::IteratorPtr; // unique_ptr<IIterator<Item>>
    using ItemSequencePtr = std::unique_ptr<MutableArraySequence<Item>>; // результат поиска по диапозону

    explicit BinarySearchTree(Less less = Less())
        : root_(nullptr), count_(0), less_(less) {}

    // Глубокая копия
    BinarySearchTree(const BinarySearchTree& other)
        : root_(copy_tree(other.root_)), count_(other.count_), less_(other.less_) {}

    BinarySearchTree& operator=(const BinarySearchTree& other) {
        if (this != &other) {
            Node* copy = copy_tree(other.root_); // сначала копия: если new бросит, *this не пострадает
            destroy(root_);
            root_ = copy;
            count_ = other.count_;
            less_ = other.less_;
        }
        return *this;
    }

    ~BinarySearchTree() override {
        destroy(root_);
    }

    int get_count() const override {
        return count_;
    }

    int get_capacity() const override {
        return count_; // У дерева нет заранее выделенного места => вместимость совпадает с числом элементов
    }

    const TValue* find(const TKey& key) const override {
        const Node* node = find_node(key);

        if (node != nullptr)
            return &node->item.value;
        return nullptr;
    }

    TValue* find(const TKey& key) override {
        const BinarySearchTree* self = this;
        return const_cast<TValue*>(self->find(key)); // объект не const => снимать const безопасно
    }

    void add(const TKey& key, const TValue& value) override {
        Node* parent = nullptr;
        Node* current = root_;
        bool go_left = false; // в какую сторону шли последний раз

        while (current != nullptr) {
            parent = current;
            if (less_(key, current->item.key)) {
                current = current->left;
                go_left = true;
            } else if (less_(current->item.key, key)) {
                current = current->right;
                go_left = false;
            } else {
                throw DuplicateKeyException();
            }
        }

        Node* node = new Node(Item(key, value), parent);
        // прикрепляем лист к родителю
        if (parent == nullptr)
            root_ = node;
        else if (go_left)
            parent->left = node;
        else
            parent->right = node;
        count_++;

        after_insert(node);
    }

    // Удаление с перевешиванием узлов
    void remove(const TKey& key) override {
        Node* del_node = const_cast<Node*>(find_node(key));
        if (del_node == nullptr)
            throw KeyNotFoundException();

        Node* rebalance_from; // самый нижний узел, у которого изменилось поддерево (передают в хук after_remove)

        if (del_node->left == nullptr || del_node->right == nullptr) {
            // 0 или 1 ребёнок: ребёнок (или nullptr) встаёт на место del_node
            Node* child = nullptr;
            if (del_node->left != nullptr)
                child = del_node->left;
            else
                child = del_node->right;

            rebalance_from = del_node->parent;
            replace_child(del_node->parent, del_node, child); // встаёт на место удаляемого узла
        } else {
            // 2 ребёнка: на место del_node встаёт преемник s - минимум правого поддерева (у s нет левого ребёнка)
            Node* s = del_node->right;
            while (s->left != nullptr)
                s = s->left;

            if (s->parent != del_node) {
                rebalance_from = s->parent;
                replace_child(s->parent, s, s->right); // вырезаем s, его правое поддерево поднимается
                s->right = del_node->right; // наслденик забирает правое поддерево
                s->right->parent = s;
            } else {
                rebalance_from = s; // s - правый ребёнок del_node, он просто поднимается
            }

            replace_child(del_node->parent, del_node, s); // преемник встаёт на место удаляемого
            s->left = del_node->left; // забрал левое поддерево
            s->left->parent = s;
            s->height = del_node->height; // для AVL - высота как у прежнего узла
        }

        delete del_node;
        count_--;

        after_remove(rebalance_from);
    }

    void clear() override {
        destroy(root_);
        root_ = nullptr;
        count_ = 0;
    }

    // Обход в порядке возрастания ключей
    IteratorPtr get_iterator() const override {
        return std::make_unique<Iterator>(leftmost(root_), less_);
    }

    /*============ Поиск по диапазону ============*/

    // Итератор по ключам из [from, to] включительно, в порядке возрастания
    IteratorPtr get_range_iterator(const TKey& from, const TKey& to) const {
        const Node* start = less_(to, from) ? nullptr : lower_bound_node(from);
        return std::make_unique<Iterator>(start, less_, to);
    }

    // Все пары с ключами из [from, to], отсортированные по ключу: O(h + k)
    // Результат уже отсортирован
    ItemSequencePtr find_range(const TKey& from, const TKey& to) const {
        ItemSequencePtr result = std::make_unique<MutableArraySequence<Item>>();
        for (IteratorPtr it = get_range_iterator(from, to); it->has_next(); it->next())
            result->append(it->get_current_item());
        return result;
    }

    // Число ключей в [from, to] (нужен там где сами пары не требуются - UI)
    int count_range(const TKey& from, const TKey& to) const {
        int result = 0;
        for (IteratorPtr it = get_range_iterator(from, to); it->has_next(); it->next())
            result++;
        return result;
    }

    /*============ Дополнительно ============*/

    const TKey& get_min_key() const {
        if (root_ == nullptr)
            throw KeyNotFoundException("Tree is empty");
        return leftmost(root_)->item.key;
    }

    const TKey& get_max_key() const {
        if (root_ == nullptr)
            throw KeyNotFoundException("Tree is empty");
        const Node* node = root_;
        while (node->right != nullptr)
            node = node->right;
        return node->item.key;
    }

    // Высота (число уровней): пустое дерево - 0, один узел - 1.
    // Для обычного BST - обход за O(n) со своим стеком; AvlTree отвечает за O(1).
    virtual int get_height() const {
        if (root_ == nullptr)
            return 0;

        MutableArraySequence<Pair<const Node*, int>> stack; // (узел, его глубина)
        stack.append(Pair<const Node*, int>(root_, 1));
        int result = 0;

        while (!stack.is_empty()) {
            Pair<const Node*, int> top = stack.get_last();
            stack.remove_at(stack.get_length() - 1);

            if (top.value > result)
                result = top.value;
            if (top.key->left != nullptr)
                stack.append(Pair<const Node*, int>(top.key->left, top.value + 1));
            if (top.key->right != nullptr)
                stack.append(Pair<const Node*, int>(top.key->right, top.value + 1));
        }
        return result;
    }

    // Проверка структуры (для тестов): порядок ключей, ссылки на родителей, число узлов
    // и инварианты наследника (у AVL - высоты и баланс).
    bool check_invariants() const {
        if (root_ != nullptr && root_->parent != nullptr)
            return false;

        int visited = 0;
        const Node* previous = nullptr;
        for (const Node* node = leftmost(root_); node != nullptr; node = successor(node)) {
            if (node->left != nullptr && node->left->parent != node)
                return false;
            if (node->right != nullptr && node->right->parent != node)
                return false;
            if (previous != nullptr && !less_(previous->item.key, node->item.key))
                return false; // in-order обход обязан давать строго возрастающие ключи
            if (!check_node(node))
                return false;
            previous = node;
            visited++;
        }
        return visited == count_;
    }

protected: // для того, чтобы наследник (avl) мог работать с узлами
    struct Node {
        Item item;
        Node* left;
        Node* right;
        Node* parent;
        int height; // используется только в AvlTree

        Node(const Item& item, Node* parent) // всегда создаёт лист
            : item(item), left(nullptr), right(nullptr), parent(parent), height(1) {}
    };

    Node* root_; // корень - точка входа в дерево
    int count_;
    Less less_; // компаратор

    // Хуки для наследников. node - только что вставленный лист.
    virtual void after_insert(Node* node) { (void)node; }
    // node - самый нижний узел с изменившимся поддеревом (может быть nullptr)
    virtual void after_remove(Node* node) { (void)node; }
    // Дополнительная проверка одного узла в check_invariants
    virtual bool check_node(const Node* node) const { (void)node; return true; }

    const Node* find_node(const TKey& key) const {
        const Node* current = root_;
        while (current != nullptr) {
            if (less_(key, current->item.key))
                current = current->left;
            else if (less_(current->item.key, key))
                current = current->right;
            else
                return current;
        }
        return nullptr;
    }

    // Ставит new_child на место old_child у parent (или в корень, если parent == nullptr)
    void replace_child(Node* parent, Node* old_child, Node* new_child) {
        if (parent == nullptr)
            root_ = new_child;
        else if (parent->left == old_child)
            parent->left = new_child;
        else
            parent->right = new_child;

        // обновление родителя у нового ребёнка
        if (new_child != nullptr)
            new_child->parent = parent;
    }

    // Первый узел с ключом >= key (или nullptr)
    const Node* lower_bound_node(const TKey& key) const {
        const Node* current = root_;
        const Node* result = nullptr;
        while (current != nullptr) {
            if (less_(current->item.key, key)) {
                current = current->right;
            } else {
                result = current;          // подходит, но слева может быть ключ поменьше
                current = current->left;
            }
        }
        return result;
    }

    static const Node* leftmost(const Node* node) { // движение влево - поиск самого маленького ключа в поддереве
        if (node == nullptr)
            return nullptr;
        while (node->left != nullptr)
            node = node->left;
        return node;
    }

    static const Node* successor(const Node* node) { // следующий по величине ключ после заданного узла (наследник)
        if (node->right != nullptr)
            return leftmost(node->right);

        // поднимаемся, пока приходим из правого поддерева
        const Node* parent = node->parent;
        while (parent != nullptr && node == parent->right) {
            node = parent;
            parent = parent->parent;
        }
        return parent;
    }

private:
    static Node* clone_node(const Node* source, Node* parent) {
        Node* node = new Node(source->item, parent);
        node->height = source->height;
        return node;
    }

    // Итеративное копирование: стек пар (узел-оригинал, его копия)
    static Node* copy_tree(const Node* source_root) {
        if (source_root == nullptr)
            return nullptr;

        Node* new_root = clone_node(source_root, nullptr);
        try {
            MutableArraySequence<Pair<const Node*, Node*>> stack;
            stack.append(Pair<const Node*, Node*>(source_root, new_root));

            while (!stack.is_empty()) {
                Pair<const Node*, Node*> top = stack.get_last();
                stack.remove_at(stack.get_length() - 1);
                const Node* source = top.key;
                Node* copy = top.value;

                if (source->left != nullptr) {
                    copy->left = clone_node(source->left, copy);
                    stack.append(Pair<const Node*, Node*>(source->left, copy->left));
                }
                if (source->right != nullptr) {
                    copy->right = clone_node(source->right, copy);
                    stack.append(Pair<const Node*, Node*>(source->right, copy->right));
                }
            }
        } catch (...) {
            destroy(new_root); // не оставляем утечку из недостроенной копии
            throw;
        }
        return new_root;
    }

    // Итеративное удаление без доп. памяти: правыми поворотами превращаем дерево
    // в "палку" вправо и удаляем её узлы по одному. Каждый узел поворачивается
    // не больше одного раза => O(n).
    static void destroy(Node* node) {
        while (node != nullptr) {
            if (node->left != nullptr) {
                Node* left = node->left;
                node->left = left->right;
                left->right = node;
                node = left;
            } else {
                Node* right = node->right;
                delete node;
                node = right;
            }
        }
    }

    /*============ Итератор (О-1.2) ============*/

    // Семантика как у HashTable::Iterator: has_next() == "есть текущий элемент".
    // Опционально ограничен сверху ключом upper (для поиска по диапазону).
    class Iterator : public IIterator<Item> {
    public:
        Iterator(const Node* start, const Less& less) // обычный обход до конца, bounded_ = false
            : current_(start), less_(less), bounded_(false), upper_() {}

        Iterator(const Node* start, const Less& less, const TKey& upper) // обход с остановкой после ключа upper
            : current_(start), less_(less), bounded_(true), upper_(upper) {}

        bool has_next() const override {
            return current_ != nullptr && (!bounded_ || !less_(upper_, current_->item.key));
        }

        bool next() override {
            if (!has_next())
                return false;
            current_ = successor(current_);
            return has_next();
        }

        const Item& get_current_item() const override {
            if (!has_next())
                throw IteratorOutOfRangeException();
            return current_->item;
        }

    private:
        const Node* current_;
        Less less_;
        bool bounded_; // есть ли верхняя граница
        TKey upper_; // верхняя граница to, используется только при bounded_ == true
    };
};
