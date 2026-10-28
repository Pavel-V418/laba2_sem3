#pragma once


// "набор IDictionary<TKey,TValue> - индексов"
// Индексы могут иметь разные типы ключей (int, string, CompositeKey...), поэтому
// храним их через общий интерфейс IIndex<TItem>, а конкретный тип получаем через get<TKey>().

#include <memory>
#include <string>
#include <stdexcept>
#include "index.h"
#include "../common/exception.h"
#include "../LR2/mutableArraySequence.h"

template<class TItem>
class IndexManager {

public:
    explicit IndexManager(const Sequence<TItem>& data)
        : data_(&data), indexes_() {}

    ~IndexManager() {
        clear();
    }

    // Владеет индексами через сырые указатели -> копирование запрещено (иначе 2 менеджера удалили бы одни и те же индексы)
    IndexManager(const IndexManager&) = delete;
    IndexManager& operator=(const IndexManager&) = delete;

    const Sequence<TItem>& get_data() const {
        return *data_;
    }

    int get_count() const {
        return indexes_.get_length();
    }

    bool contains(const std::string& name) const {
        return position_of(name) >= 0;
    }

    // Построить новый индекс и добавить в набор. Пример:
    //     manager.add_index<int>("birth_year", [](const Person& p) { return p.get_birth_year(); },
    //                            IndexKind::AvlTree);
    template<class TKey, class Less = KeyLess<TKey>, class F>
    Index<TItem, TKey, Less>& add_index(const std::string& name, F extractor, IndexKind kind, Less less = Less()) {
        if (contains(name))
            throw DuplicateKeyException("IndexManager: index '" + name + "' already exists");

        auto index = std::make_unique<Index<TItem, TKey, Less>>(name, *data_, extractor, kind, less); // создаём индекс в куче
        Index<TItem, TKey, Less>& result = *index; // ссылка индекса
        indexes_.append(index.get());
        index.release();   // юникптр отказывается от владения и теперь индексом владеет indexes_
        return result;
    }

    IIndex<TItem>& get_index(const std::string& name) const {
        int pos = position_of(name);
        if (pos < 0)
            throw KeyNotFoundException("IndexManager: no index '" + name + "'");
        return *indexes_.get(pos);
    }

    IIndex<TItem>& get_index(int number) const {
        if (number < 0 || number >= get_count())
            throw std::out_of_range("IndexManager: index number out of range");
        return *indexes_.get(number);
    }

    // Индекс с конкретным типом ключа (для поиска). Тип не совпал -> исключение.
    template<class TKey, class Less = KeyLess<TKey>>
    Index<TItem, TKey, Less>& get(const std::string& name) const {
        auto* typed = dynamic_cast<Index<TItem, TKey, Less>*>(&get_index(name));
        if (typed == nullptr)
            throw std::invalid_argument("IndexManager: index '" + name + "' has another key type");
        return *typed;
    }

    void remove(const std::string& name) {
        int pos = position_of(name); // находит индекс по имени
        if (pos < 0)
            throw KeyNotFoundException("IndexManager: no index '" + name + "'");
        delete indexes_.get(pos);
        indexes_.remove_at(pos);
    }

    void clear() {
        for (int i = 0; i < indexes_.get_length(); i++)
            delete indexes_.get(i);
        indexes_ = MutableArraySequence<IIndex<TItem>*>();
    }

    // В коллекцию дописан объект на позицию position - обновляем все индексы за O(вставки)
    void notify_appended(int position) {
        for (int i = 0; i < indexes_.get_length(); i++)
            indexes_.get(i)->add_item(position);
    }

    // Коллекция изменилась целиком (сгенерирована / загружена заново)
    void rebuild_all() {
        for (int i = 0; i < indexes_.get_length(); i++)
            indexes_.get(i)->rebuild();
    }

private:
    const Sequence<TItem>* data_;
    MutableArraySequence<IIndex<TItem>*> indexes_;  // список указателей на индексы

    int position_of(const std::string& name) const {
        for (int i = 0; i < indexes_.get_length(); i++)
            if (indexes_.get(i)->get_name() == name)
                return i;
        return -1;
    }
};
