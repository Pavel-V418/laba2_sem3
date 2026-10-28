#pragma once

// Индекс - словарь IDictionary<TKey, PositionList>:
// ключ - значение атрибута (год рождения, фамилия, составной ключ из блока 5...);
// значение - позиции ВСЕХ объектов коллекции с таким значением атрибута.
#include <memory>
#include <string>
#include <stdexcept>
#include <utility>   // std::move для unique_ptr

#include "key_extractor.h"
#include "../dictionary/i_dictionary.h"
#include "../dictionary/sequence_dictionary.h"
#include "../dictionary/hash_table.h"
#include "../tree/binary_search_tree.h"
#include "../tree/avl_tree.h"
#include "../common/comparators.h"
#include "../LR2/sequence.h"
#include "../LR2/mutableArraySequence.h"

// Какой структурой реализован индекс
enum class IndexKind {
    Sequence,     // SequenceDictionary - наивный линейный словарь (база для сравнения)
    BinaryTree,   // BinarySearchTree   (М-3)
    AvlTree,      // AvlTree            (М-3.1)
    HashTable     // HashTable          (М-5, М-5.1)
};

static constexpr int INDEX_KIND_COUNT = 4; // число вариантов, чтобы перебирать их в цикле(тесты, бенчмарки)

inline const char* to_string(IndexKind kind) {
    switch (kind) {
        case IndexKind::Sequence:
            return "Sequence";
        case IndexKind::BinaryTree:
            return "BST";
        case IndexKind::AvlTree:
            return "AVL";
        case IndexKind::HashTable:
            return "HashTable";
    }
    return "unknown";
}

// Умеет ли исходная структура быстрый поиск по диапозону и обход по порядку ключей
// Деревья умеют искать по диапазону за O(h + k); остальные - только полным перебором O(n)
inline bool is_ordered(IndexKind kind) {
    return kind == IndexKind::BinaryTree || kind == IndexKind::AvlTree;
}

using PositionList = MutableArraySequence<int>; // значение в словаре индексов
using PositionListPtr = std::unique_ptr<PositionList>; // результат поиска по диапозону

/*============ Общий интерфейс (без типа ключа) ============*/

// Нужен IndexManager'у, чтобы хранить индексы с РАЗНЫМИ типами ключей в одном списке
template<class TItem>
class IIndex {
public:
    virtual ~IIndex() = default;

    virtual const std::string& get_name() const = 0;
    virtual IndexKind get_kind() const = 0;

    virtual int get_key_count() const = 0;    // число различных значений атрибута
    virtual int get_item_count() const = 0;   // сколько объектов проиндексировано
    virtual int get_height() const = 0;       // высота дерева (0 - не дерево)

    virtual void add_item(int position) = 0;  // в коллекцию добавили объект на позицию position
    virtual void rebuild() = 0;               // коллекция изменилась целиком - строим заново
};

/*============ Индекс с конкретным типом ключа ============*/

// TItem - тип объектов коллекции (Person)
// TKey - тип атрибута
// Less - операция сравнения для деревьев
template<class TItem, class TKey, class Less = KeyLess<TKey>>
class Index : public IIndex<TItem> {

public:
    using Dictionary = IDictionary<TKey, PositionList>; // словарь (базовый класс - не видны методы наследников)
    using Tree = BinarySearchTree<TKey, PositionList, Less>; // словарь, если он дерево (нужен для поиска по диапозону)

    // name - имя индекса (для меню и поиска в IndexManager);
    // data - коллекция; индекс хранит ссылку на неё и не владеет ею;
    // extractor - функция/лямбда TKey(const TItem&) - "какой атрибут индексируем".
    template<class F>
    Index(const std::string& name, const Sequence<TItem>& data, F extractor,
          IndexKind kind, Less less = Less())
        : name_(name),
          data_(&data),
          extractor_(std::make_unique<FunctionKeyExtractor<TItem, TKey, F>>(extractor)),
          kind_(kind),
          less_(less),
          dictionary_(), // Словаря ещё нет. Его создаёт и заполняет rebuild() в теле конструктора
          tree_(nullptr),
          item_count_(0) {
        rebuild();
    }

    // Индекс ссылается на коллекцию и владеет словарём - копировать его незачем
    // словарь и извлекатель лежат в unique_ptr и не копируются
    Index(const Index&) = delete;
    Index& operator=(const Index&) = delete;

    const std::string& get_name() const override { return name_; }
    IndexKind get_kind() const override { return kind_; }
    int get_key_count() const override { return dictionary_->get_count(); }
    int get_item_count() const override { return item_count_; }

    int get_height() const override {
        return tree_ != nullptr ? tree_->get_height() : 0;
    }

    // Значение атрибута у объекта
    TKey key_of(const TItem& item) const {
        return extractor_->extract(item);
    }

    /*============ Поиск ============*/
    const PositionList* find(const TKey& key) const {
        return dictionary_->find(key);
    }

    int count(const TKey& key) const { // сколько объектов с таким ключом
        const PositionList* positions = find(key);
        return positions != nullptr ? positions->get_length() : 0;
    }

    bool contains(const TKey& key) const {
        return find(key) != nullptr;
    }

    // Поиск по диапазону [from, to] включительно
    // дерево: O(h + k), позиции идут в порядке возрастания ключа;
    // хеш / Sequence: полный перебор словаря O(n), порядок произвольный.
    // Если to < from - результат пуст.
    PositionListPtr find_range(const TKey& from, const TKey& to) const {
        PositionListPtr result = std::make_unique<PositionList>();
        if (less_(to, from))
            return result;

        if (tree_ != nullptr) {
            for (auto it = tree_->get_range_iterator(from, to); it->has_next(); it->next()) // порядок есть
                append_all(*result, it->get_current_item().value);
        } else {
            for (auto it = dictionary_->get_iterator(); it->has_next(); it->next()) {
                const auto& item = it->get_current_item(); // порядка ключей нет, так что:
                if (!less_(item.key, from) && !less_(to, item.key)) // берём пары, попавшие в диап
                    append_all(*result, item.value);
            }
        }
        return result;
    }

    int count_range(const TKey& from, const TKey& to) const {
        return find_range(from, to)->get_length();
    }

    // Все различные значения атрибута (у деревьев - по возрастанию)
    std::unique_ptr<MutableArraySequence<TKey>> get_keys() const {
        return dictionary_->get_keys();
    }

    /*============ Обновление ============*/

    // В коллекцию добавлен новый объект: дописываем его, не перестраивая индекс
    void add_item(int position) override {
        if (position < 0 || position >= data_->get_length())
            throw std::out_of_range("Index::add_item: position out of range");
        insert(key_of(data_->get(position)), position);
    }

    // Полная перестройка: O(n * стоимость вставки)
    void rebuild() override {
        create_dictionary();
        item_count_ = 0;

        // Пройти коллекцию перечислителем от начала до конца, считая позицию. (обход по порядку)
        // Для каждого человека вычислить ключ и вставить пару "ключ, позиция".
        std::unique_ptr<IEnumerator<TItem>> it(data_->get_enumerator());
        int position = 0;
        while (it->has_more_elements())
            insert(key_of(it->next()), position++);
    }

private:
    std::string name_; // имя индекса (по нему IndexManager его находит)
    const Sequence<TItem>* data_; // указатель на коллекцию (людей)
    std::unique_ptr<IKeyExtractor<TItem, TKey>> extractor_; // указатель на интерфейс извлекателя
    IndexKind kind_; // тип структуры
    Less less_; // нужен для деревьев и проверки диапозонов
    std::unique_ptr<Dictionary> dictionary_; // сам индекс - словарь "значение атрибута → позиции"
    Tree* tree_; // тот же объект, что dictionary_, если это дерево (нужен для диапазона)
    int item_count_; // сколько объекторв проиндексировано

    void insert(const TKey& key, int position) {
        // ключ новый -> добавится пустой список; затем дописываем позицию
        dictionary_->get_or_add(key).append(position); // находит список позиций для ключа и если его нет - создаёт пустой список
        item_count_++;
    }

    void create_dictionary() {
        tree_ = nullptr;
        switch (kind_) {
            case IndexKind::Sequence:
                dictionary_ = std::make_unique<SequenceDictionary<TKey, PositionList>>();
                break;
            case IndexKind::BinaryTree: {
                auto tree = std::make_unique<BinarySearchTree<TKey, PositionList, Less>>(less_);
                tree_ = tree.get();
                dictionary_ = std::move(tree);
                break;
            }
            case IndexKind::AvlTree: {
                auto tree = std::make_unique<AvlTree<TKey, PositionList, Less>>(less_);
                tree_ = tree.get();
                dictionary_ = std::move(tree);
                break;
            }
            case IndexKind::HashTable:
                dictionary_ = std::make_unique<HashTable<TKey, PositionList>>();
                break;
            default:
                throw std::invalid_argument("Index: unknown index kind");
        }
    }

    static void append_all(PositionList& target, const PositionList& source) {
        for (int i = 0; i < source.get_length(); i++)
            target.append(source.get(i));
    }
};
