#ifndef LABA2_DYNAMICARRAY_H
#define LABA2_DYNAMICARRAY_H

#include <stdexcept>
#include "i_enumerator.h"

template <class T>
class DynamicArray {
private:
    T *data;
    int size;
    int capacity;

public:
    // constructors - способы создания массива
    DynamicArray(const T *items, int count);
    explicit DynamicArray(int size);
    DynamicArray(const DynamicArray<T>& dynamicArray);

    // оператор присваивания (правило трёх: без него копирование = двойное delete[])
    DynamicArray<T>& operator=(const DynamicArray<T>& other);

    // destructor
    ~DynamicArray();

    // Decomposition
    const T& get(int index) const;
    int get_size() const;
    int get_capacity() const;

    // operations
    void set(const T& value, int index);
    void resize(int newSize);

    // итератор
    // ВНИМАНИЕ: становится недействительным после resize (массив переезжает)
    class ArrayEnumerator : public IEnumerator<T> {

    public:
        ArrayEnumerator(const DynamicArray<T> *arr)
            : current(arr->data), // указатель на первый элемент
              end(arr->data + arr->get_size()) {} // указатель на конец

        bool has_more_elements() override {
            return current != end;
        }

        const T& next() override {
            if (!has_more_elements())
                throw std::out_of_range("ArrayEnumerator::next");

            return *current++; // разыменовали и сдвинулись
        }

    private:
        const T* current;
        const T* end;
    };

};

template<class T>
DynamicArray<T>::DynamicArray(const T *items, int count) {
    if (count < 0)
        throw std::invalid_argument("Size cannot be negative");

    this->size = count;
    capacity = (count > 0) ? count : 1;

    data = new T[capacity];

    for (int i = 0; i < size; i++)
        data[i] = items[i];
}

template<class T>
DynamicArray<T>::DynamicArray(int size) {
    if (size < 0)
        throw std::invalid_argument("Size cannot be negative");

    this->size = size;
    capacity = (size > 0) ? size : 1;

    data = new T[capacity];
}

template<class T>
DynamicArray<T>::DynamicArray(const DynamicArray<T>& dynamicArray) {
    this->size = dynamicArray.size;
    capacity = dynamicArray.capacity;

    data = new T[capacity];

    for (int i = 0; i < size; i++)
        data[i] = dynamicArray.data[i];
}

template<class T>
DynamicArray<T>& DynamicArray<T>::operator=(const DynamicArray<T>& other) {
    if (this == &other)
        return *this;

    // сначала копируем, потом удаляем старое: если new бросит исключение,
    // объект останется в корректном состоянии
    T *newData = new T[other.capacity];
    for (int i = 0; i < other.size; i++)
        newData[i] = other.data[i];

    delete[] data;
    data = newData;
    size = other.size;
    capacity = other.capacity;

    return *this;
}

template<class T>
DynamicArray<T>::~DynamicArray() {
    delete[] data;
}

template<class T>
const T& DynamicArray<T>::get(int index) const {
    if (index < 0 || index >= size)
        throw std::out_of_range("Index out of range");
    return data[index];
}

template<class T>
int DynamicArray<T>::get_size() const {
    return size;
}

template<class T>
int DynamicArray<T>::get_capacity() const {
    return capacity;
}

template<class T>
void DynamicArray<T>::set(const T& value, int index) {
    if (index < 0 || index >= size)
        throw std::out_of_range("Index out of range");
    data[index] = value;
}

template<class T>
void DynamicArray<T>::resize(int newSize) {
    if (newSize < 0)
        throw std::invalid_argument("Size cannot be negative");

    if (newSize <= capacity) {
        size = newSize;
        return;
    }

    int newCapacity = capacity;

    while (newCapacity < newSize)
        newCapacity *= 2;

    T* newData = new T[newCapacity];

    for (int i = 0; i < size; i++)
        newData[i] = data[i];

    delete[] data;
    data = newData;
    capacity = newCapacity;
    size = newSize;
}

#endif //LABA2_DYNAMICARRAY_H
