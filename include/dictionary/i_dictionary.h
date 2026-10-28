#pragma once

#include <memory>
#include "../common/pair.h"
#include "../common/exception.h"
#include "i_iterator.h"
#include "../LR2/option.h"
#include "../LR2/mutableArraySequence.h"

template<class TKey, class TValue>
class IDictionary {

public:
    using Item = Pair<TKey, TValue>;
    using IteratorPtr = std::unique_ptr<IIterator<Item>>;

    virtual ~IDictionary() = default;

    virtual int get_count() const = 0;
    virtual int get_capacity() const = 0;
    bool is_empty() const {
        return get_count() == 0;
    }

    virtual const TValue* find(const TKey& key) const = 0; // читать
    virtual TValue* find(const TKey& key) = 0; // менять

    virtual void add(const TKey& key, const TValue& value) = 0;
    virtual void remove(const TKey& key) = 0;

    virtual void clear() = 0;
    virtual IteratorPtr get_iterator() const = 0;

    // работают одинаково в любой структуре

    // есть ли ключ
    bool contains_key(const TKey& key) const {
        return find(key) != nullptr;
    }

    const TValue& get(const TKey& key) const { // полученное значение можно только читать
        const TValue* value = find(key);
        if (value == nullptr)
            throw KeyNotFoundException();
        return *value;
    }

    TValue& get(const TKey& key) { // полученное значение можно менять прямо внутри словаря
        TValue* value = find(key);
        if (value == nullptr)
            throw KeyNotFoundException();
        return *value;
    }

    Option<TValue> try_get(const TKey& key) const {
        const TValue* value = find(key);
        if (value == nullptr)
            return Option<TValue>();
        return Option<TValue>(*value);
    }

    void set(const TKey& key, const TValue& value) {
        TValue *place = find(key);
        if (place != nullptr)
            *place = value;
        else
            add(key, value);
    }

    TValue& get_or_add(const TKey& key, const TValue& default_value = TValue()) {
        TValue* place = find(key);
        if (place != nullptr)
            return *place;

        add(key, default_value);
        return *find(key);
    }

    std::unique_ptr<MutableArraySequence<TKey>> get_keys() const {
        auto keys = std::make_unique<MutableArraySequence<TKey>>();
        for (IteratorPtr it = get_iterator(); it->has_next(); it->next()) {
            keys->append(it->get_current_item().key);
        }
        return keys;
    }

    std::unique_ptr<MutableArraySequence<TValue>> get_values() const {
        auto values = std::make_unique<MutableArraySequence<TValue>>();
        for (IteratorPtr it = get_iterator(); it->has_next(); it->next()) {
            values->append(it->get_current_item().value);
        }
        return values;
    }
};