#ifndef LABA2_ARRAYSEQUENCE_H
#define LABA2_ARRAYSEQUENCE_H

#include "dynamicArray.h"
#include "sequence.h"

template <class T>
class ArraySequence : public Sequence<T> {

public:

    // constructors
    ArraySequence(const T *items, int count);
    ArraySequence();
    ArraySequence(const ArraySequence<T> &arraySequence);

    // оператор присваивания (правило трёх)
    ArraySequence<T>& operator=(const ArraySequence<T> &other);

    // destructor
    ~ArraySequence() override;

    IEnumerator<T> *get_enumerator() const override;

    // Getters
    const T& get_first() const override;
    const T& get_last() const override;
    const T& get(int index) const override;

    int get_length() const override;
    int get_capacity() const;

protected:
    // Operations
    void append_internal(const T& item) override;
    void prepend_internal(const T& item) override;
    void insert_at_internal(const T& item, int index) override;
    void remove_at_internal(int index) override;
    void set_internal(const T& item, int index) override;

private:
    DynamicArray<T> *items;

};


/*============ КОНСТРУКТОРЫ ============*/
template<class T>
ArraySequence<T>::ArraySequence(const T *items, int count) {
    this->items = new DynamicArray<T>(items, count);
}

template<class T>
ArraySequence<T>::ArraySequence() {
    items = new DynamicArray<T>(0);
}

template<class T>
ArraySequence<T>::ArraySequence(const ArraySequence& arraySequence)
    : Sequence<T>() {
    items = new DynamicArray<T>(*arraySequence.items);
}

template<class T>
ArraySequence<T>& ArraySequence<T>::operator=(const ArraySequence<T> &other) {
    if (this != &other) {
        DynamicArray<T> *copy = new DynamicArray<T>(*other.items);
        delete items;
        items = copy;
    }
    return *this;
}

/*============ ДЕСТРУКТОР ============ */
template<class T>
ArraySequence<T>::~ArraySequence() {
    delete items;
}

/*============ Итератор ============ */
template<class T>
IEnumerator<T>* ArraySequence<T>::get_enumerator() const {
    return new typename DynamicArray<T>::ArrayEnumerator(this->items);
}

/*============ ГЕТТЕРЫ ============ */

template<class T>
const T& ArraySequence<T>::get_first() const {
    if (items->get_size() == 0)
        throw std::out_of_range("Sequence is empty");
    return items->get(0);
}

template<class T>
const T& ArraySequence<T>::get_last() const {
    if (items->get_size() == 0)
        throw std::out_of_range("Sequence is empty");
    return items->get(items->get_size() - 1);
}

template<class T>
const T& ArraySequence<T>::get(int index) const {
    if (index < 0 || index >= items->get_size())
        throw std::out_of_range("Index out of range");
    return items->get(index);
}

template<class T>
int ArraySequence<T>::get_length() const {
    return items->get_size();
}

template<class T>
int ArraySequence<T>::get_capacity() const {
    return items->get_capacity();
}

/*============ ОПЕРАЦИИ ============ */

template<class T>
void ArraySequence<T>::append_internal(const T& item) {
    int index_free = this->items->get_size();
    T value = item;   // item может ссылаться внутрь этого же массива

    this->items->resize(index_free + 1);
    this->items->set(value, index_free);
}

template<class T>
void ArraySequence<T>::prepend_internal(const T& item) {
    insert_at_internal(item, 0);
}

template<class T>
void ArraySequence<T>::insert_at_internal(const T& item, int index) {
    int size = this->items->get_size();

    if (index < 0 || index > size)
        throw std::out_of_range("Index out of range");

    // копия на случай, если item ссылается на элемент этого же массива
    // (после resize ссылка станет висячей)
    T value = item;

    this->items->resize(size + 1);

    for (int i = size; i > index; i--)
        this->items->set(this->items->get(i - 1), i);

    this->items->set(value, index);
}

template<class T>
void ArraySequence<T>::remove_at_internal(int index) {
    int size = this->items->get_size();

    if (index < 0 || index >= size)
        throw std::out_of_range("Index out of range");

    for (int i = index; i < size - 1; i++)
        this->items->set(this->items->get(i + 1), i);

    this->items->resize(size - 1);
}

template<class T>
void ArraySequence<T>::set_internal(const T& item, int index) {
    this->items->set(item, index);   // DynamicArray сам проверяет границы
}

#endif //LABA2_ARRAYSEQUENCE_H
