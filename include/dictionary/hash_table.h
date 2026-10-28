#pragma once

#include <memory>
#include <stdexcept>
#include <utility>   // std::move для unique_ptr
#include "i_dictionary.h"
#include "../hash/hash_functions.h"
#include "../LR2/dynamicArray.h"
#include "../LR2/mutableArraySequence.h"

template<class TKey, class TValue, class Hash = Hasher<TKey>>
class HashTable : public IDictionary<TKey, TValue> {

public:
    using Item = typename IDictionary<TKey, TValue>::Item;
    using IteratorPtr = typename IDictionary<TKey, TValue>::IteratorPtr;

    // константы
    static constexpr int DEFAULT_CAPACITY = 8; // начальное число корзин
    static constexpr double DEFAULT_P = 4.0; // порог сжатия
    static constexpr double DEFAULT_Q = 2.0; // во сколько раз изменяем размер хеш-таблицы
    static constexpr int MAX_CHAIN_LENGTH = 8; // цепочка длиннее считается "плохой коллизией"

    // static constexpr значит, что это константы класса, известные на этапе компиляции, общие для всех объектов.

    explicit HashTable(int initial_capacity = DEFAULT_CAPACITY, double p = DEFAULT_P, double q = DEFAULT_Q,Hash hash = Hash())
        : buckets_(),
          count_(0),
          min_capacity_(initial_capacity),
          p_(p),
          q_(q),
          hash_(hash),
          seed_(INITIAL_SEED),
          seed_changed_since_resize_(false),
          rebuild_count_(0) {
        if (initial_capacity < 1)
            throw std::invalid_argument("HashTable: initial capacity must be positive");
        if (!(q > 1.0) || !(p >= q)) // условие p >=q > 1
            throw std::invalid_argument("HashTable: parameters must satisfy p >= q > 1");

        buckets_ = std::make_unique<Buckets>(initial_capacity);
    }

    // Глубокое копирование (у DynamicArray и ArraySequence оно уже реализовано)
    HashTable(const HashTable& other)
        : buckets_(std::make_unique<Buckets>(*other.buckets_)),
          count_(other.count_),
          min_capacity_(other.min_capacity_),
          p_(other.p_),
          q_(other.q_),
          hash_(other.hash_),
          seed_(other.seed_),
          seed_changed_since_resize_(other.seed_changed_since_resize_),
          rebuild_count_(other.rebuild_count_) {}

    HashTable& operator=(const HashTable& other) {
        if (this != &other) {
            std::unique_ptr<Buckets> copy = std::make_unique<Buckets>(*other.buckets_);
            buckets_ = std::move(copy);   // старый массив удалит unique_ptr
            count_ = other.count_;
            min_capacity_ = other.min_capacity_;
            p_ = other.p_;
            q_ = other.q_;
            hash_ = other.hash_;
            seed_ = other.seed_;
            seed_changed_since_resize_ = other.seed_changed_since_resize_;
            rebuild_count_ = other.rebuild_count_;
        }
        return *this;
    }

    int get_count() const override {
        return count_;
    }

    // Вместимость = число корзин: столько элементов помещается без перестройки (n < c)
    int get_capacity() const override {
        return buckets_->get_size();
    }

    const TValue* find(const TKey& key) const override {
        const Bucket& bucket = buckets_->get(index_for(key));
        int pos = position_in_bucket(bucket, key);
        if (pos < 0)
            return nullptr;
        return &bucket.get(pos).value;
    }

    TValue* find(const TKey& key) override {
        const HashTable* self = this;
        return const_cast<TValue*>(self->find(key)); // сам объект не const => снимать const безопасно
    }

    void add(const TKey& key, const TValue& value) override {
        int index = index_for(key);
        Bucket& bucket = bucket_at(index);
        if (position_in_bucket(bucket, key) >= 0)
            throw DuplicateKeyException();

        bucket.append(Item(key, value));
        count_++;

        if (count_ >= get_capacity()) {  // n == c => расширяем в q раз
            resize(grown_capacity());
        } else if (bucket.get_length() > MAX_CHAIN_LENGTH && !seed_changed_since_resize_) {
            //  слишком длинная цепочка => перестраиваем хеш-функцию
            seed_ = next_seed(seed_);
            rehash(get_capacity());
            seed_changed_since_resize_ = true;
        }
    }

    void remove(const TKey& key) override {
        Bucket& bucket = bucket_at(index_for(key));
        int pos = position_in_bucket(bucket, key);
        if (pos < 0)
            throw KeyNotFoundException();

        // порядок внутри цепочки не важен: ставим последний элемент на место удаляемого - O(1)
        int last = bucket.get_length() - 1;
        if (pos != last)
            bucket.set(bucket.get(last), pos);
        bucket.remove_at(last);
        count_--;

        // n <= c / p -> сжимаем в q раз
        if (count_ * p_ <= get_capacity()) {
            int new_capacity = shrunk_capacity();
            if (new_capacity < get_capacity())
                resize(new_capacity);
        }
    }

    void clear() override {
        buckets_ = std::make_unique<Buckets>(min_capacity_);
        count_ = 0;
        seed_changed_since_resize_ = false;
    }

    // итератор становится недействительным после add/remove/clear
    // (корзины могут переехать в новый массив при перестройке)
    IteratorPtr get_iterator() const override {
        return std::make_unique<Iterator>(*buckets_);
    }

    // Статистика (для тестов и бенчмарков)

    double get_p() const { return p_; }
    double get_q() const { return q_; }

    // n/c коэффициент
    double get_load_factor() const {
        return static_cast<double>(count_) / get_capacity();
    }

    // самая длинная цепочка
    int get_max_chain_length() const {
        int result = 0;
        for (int i = 0; i < buckets_->get_size(); i++)
            if (buckets_->get(i).get_length() > result)
                result = buckets_->get(i).get_length();
        return result;
    }

    // Сколько раз таблица перекладывала все элементы (расширение, сжатие, смена seed)
    int get_rebuild_count() const {
        return rebuild_count_;
    }

private:
    using Bucket = MutableArraySequence<Item>; // корзина
    using Buckets = DynamicArray<Bucket>; // массив корзин (для адресации по индексу)

    static constexpr HashCode INITIAL_SEED = 0;

    std::unique_ptr<Buckets> buckets_; // указатель: при перестройке просто подменяем массив, без копирования
    int count_;
    int min_capacity_; // ниже этой вместимости не сжимаемся
    double p_; // порог сжатия
    double q_; // шаг изменения размера
    Hash hash_; // хешер
    HashCode seed_; // seed хеш-функции, меняется при перестройке из-за коллизий
    bool seed_changed_since_resize_; // предохранитель от O(n^2) перестроек (разрешает менять seed не более 1 раза м.у изменениями размера)
    int rebuild_count_; // статистика для тестов

    // Хеш-функция: второй этап
    // 1. Для текущего размера таблицы
    int index_for(const TKey& key) const {
        return index_for(key, get_capacity());
    }

    // возврат номера корзины
    // 2. Для произвольного размера таблицы (нужен для rehash)
    int index_for(const  TKey& key, int capacity) const {
        HashCode h = mix_hash(hash_(key) ^ seed_); // unsigned int
        return static_cast<int>(h % static_cast<HashCode>(capacity)); // превращает большое число h в номер корзины
    }

    static HashCode next_seed(HashCode seed) {
        return seed * 1664525u + 1013904223u; // линейный конгруэнтный генератор (LCG)
    }

    /* Вспомогательные функции для корзин */

    // Возвращает корзину с номером index так, чтобы её можно было менять
    Bucket& bucket_at(int index) {
        return const_cast<Bucket&>(buckets_->get(index));
    }
    // линейный поиск ключа внутри корзины
    static int position_in_bucket(const Bucket& bucket, const TKey& key) {
        for (int i = 0; i < bucket.get_length(); i++)
            if (bucket.get(i).key == key)
                return i;
        return -1;
    }

    /*============ Перестройка ============*/

    int grown_capacity() const {
        int capacity = get_capacity();
        int result = static_cast<int>(capacity * q_);
        if (result <= capacity)   // при q, близком к 1, и малом c округление могло "съесть" рост
            result = capacity + 1;
        return result;
    }

    int shrunk_capacity() const {
        int result = static_cast<int>(get_capacity() / q_);
        if (result < min_capacity_)
            result = min_capacity_;
        return result;
    }

    void resize(int new_capacity) {
        rehash(new_capacity);
        seed_changed_since_resize_ = false;
    }

    // Перекладываем все элементы в новый массив корзин: O(n + c)
    void rehash(int new_capacity) {
        std::unique_ptr<Buckets> new_buckets = std::make_unique<Buckets>(new_capacity);

        for (int i = 0; i < buckets_->get_size(); i++) {
            const Bucket& old_bucket = buckets_->get(i);
            for (int j = 0; j < old_bucket.get_length(); j++) {
                const Item& item = old_bucket.get(j);
                int index = index_for(item.key, new_capacity);
                const_cast<Bucket&>(new_buckets->get(index)).append(item);
            }
        }

        buckets_ = std::move(new_buckets);
        rebuild_count_++;
    }

    /*============ Итератор (О-1.4) ============*/

    // Обходит корзины по порядку, пропуская пустые. Порядок элементов - произвольный.
    // Семантика как у SequenceDictionary::Iterator: has_next() == "есть текущий элемент".
    class Iterator : public IIterator<Item> {
    public:
        explicit Iterator(const Buckets& buckets)
            : buckets_(buckets), bucket_(0), position_(0) {
            skip_empty_buckets();
        }

        bool has_next() const override {
            return bucket_ < buckets_.get_size();
        }

        bool next() override {
            if (!has_next())
                return false;

            position_++;
            if (position_ >= buckets_.get(bucket_).get_length()) {
                bucket_++;
                position_ = 0;
                skip_empty_buckets();
            }
            return has_next();
        }

        const Item& get_current_item() const override {
            if (!has_next())
                throw IteratorOutOfRangeException();
            return buckets_.get(bucket_).get(position_);
        }

    private:
        const Buckets& buckets_; // на какой массив смотрим
        int bucket_; // номер текущей корзины
        int position_; // номер элемента внутри корзины

        void skip_empty_buckets() {
            while (bucket_ < buckets_.get_size() && buckets_.get(bucket_).get_length() == 0)
                bucket_++;
        }
    };
};
