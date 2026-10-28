#pragma once

#include "i_dictionary.h"

template<class TKey, class TValue>
class SequenceDictionary : public IDictionary<TKey, TValue> {

public:
    using Item = typename IDictionary<TKey, TValue>::Item;
    using IteratorPtr = typename IDictionary<TKey, TValue>::IteratorPtr;

    SequenceDictionary()
        : items_() {}

    int get_count() const override {
        return items_.get_length();
    }

    int get_capacity() const override {
        return items_.get_capacity();
    }

    const TValue* find(const TKey& key) const override {
        int index = index_of(key);
        if (index < 0)
            return nullptr;
        return &items_.get(index).value;
    }

    TValue* find(const TKey& key) override {
        int index = index_of(key);
        if (index < 0)
            return nullptr;
        return const_cast<TValue*>(&items_.get(index).value); // const_cast снимает const (разрешено писать по этому адресу)
    }

    void add(const TKey& key, const TValue& value) override {
        if (index_of(key) >= 0)
            throw DuplicateKeyException();
        items_.append(Item(key, value));
    }

    void remove(const TKey& key) override {
        int index = index_of(key);
        if (index < 0)
            throw KeyNotFoundException();

        int last = items_.get_length() - 1;
        if (index != last)
            items_.set(items_.get(last), index); // порядок в словаре не важен => на место удаляемого вписываем последний
        items_.remove_at(last);
    }

    void clear() override {
        items_ = MutableArraySequence<Item>(); // очистка старого MutArrSeq реализована в операторе "=" в ArraySequence
    }

    IteratorPtr get_iterator() const override {
        return std::make_unique<Iterator>(items_);
    }

private:
    MutableArraySequence<Item> items_;

    // линейный поиск
    int index_of(const TKey& key) const {
        for (int i = 0; i < items_.get_length(); i++)
            if (items_.get(i).key == key)
                return i;
        return -1;
    }

    class Iterator : public IIterator<Item> {
    public:
        explicit Iterator(const MutableArraySequence<Item>& items)
            : items_(items), index_(0) {}

        bool has_next() const override {
            return index_ < items_.get_length();
        }

        bool next() override {
            if (!has_next())
                return false;
            index_++;
            return has_next();
        }

        const Item& get_current_item() const override {
            if (!has_next())
                throw IteratorOutOfRangeException();
            return items_.get(index_);
        }

    private:
        const MutableArraySequence<Item>& items_; // ссылка на массив, в котором хранятся пары
        int index_; // номер текущей пары
    };

};