#pragma once

#include <string>
#include <type_traits>   // std::decay_t - только вывод типов при компиляции, не контейнер

// Составной ключ (кортеж) из произвольного числа компонент:
//     CompositeKey<int, std::string>ж - (год рождения, фамилия)
//     CompositeKey<std::string, std::string, int> - (фамилия, имя, год)
//
// Устроен рекурсивно, как односвязный список, только на уровне ТИПОВ:
//     CompositeKey<A, B, C> = head (A) + tail (CompositeKey<B, C>)
//     CompositeKey<B, C> = head (B) + tail (CompositeKey<C>)
//     CompositeKey<C> = head (C) + tail (CompositeKey<>)
//     CompositeKey<> - пустой ключ, "nullptr" этого списка, конец рекурсии
// Вся рекурсия раскрывается компилятором, в рантайме это обычная структура из полей.

template<class... Ts> // любое число типов
struct CompositeKey;

// Пустой ключ: конец рекурсии
template<>
struct CompositeKey<> {
    static constexpr int SIZE = 0;

    // все пустые ключи равны друг другу
    bool operator<(const CompositeKey&) const { return false; }
    bool operator==(const CompositeKey&) const { return true; }
};

/*============ Непустой ключ: голова + хвост ============*/

template<class Head, class... Tail>
struct CompositeKey<Head, Tail...> {
    static constexpr int SIZE = 1 + sizeof...(Tail); // число компонент

    Head head;                    // первая компонента
    CompositeKey<Tail...> tail;   // остальные

    // нужен DynamicArray (new T[n])
    CompositeKey()
        : head(), tail() {}

    // explicit: чтобы int случайно не превращался в CompositeKey<int>
    explicit CompositeKey(const Head& first, const Tail&... rest)
        : head(first), tail(rest...) {}

    // Доступ к компоненте по номеру: key.get<0>(), key.get<1>()...
    // Номер проверяется при компиляции; if constexpr отбрасывает ненужную ветку.
    template<int I> // параметр - число
    const auto& get() const {
        static_assert(I >= 0 && I < SIZE, "CompositeKey::get: index out of range"); //проверка компилятора
        if constexpr (I == 0)
            return head;
        else
            return tail.template get<I - 1>();
    }

    template<int I>
    auto& get() {
        static_assert(I >= 0 && I < SIZE, "CompositeKey::get: index out of range");
        if constexpr (I == 0)
            return head;
        else
            return tail.template get<I - 1>();
    }

    // Лексикографический порядок (как слова в словаре):
    // решает первая компонента, на которой ключи различаются.
    bool operator<(const CompositeKey& other) const {
        if (head < other.head)
            return true;
        if (other.head < head)
            return false;
        return tail < other.tail;   // головы равны -> сравниваем хвосты
    }

    bool operator==(const CompositeKey& other) const {
        return head == other.head && tail == other.tail;
    }

    // Остальные сравнения выражаются через < и ==
    bool operator!=(const CompositeKey& other) const { return !(*this == other); }
    bool operator>(const CompositeKey& other) const { return other < *this; }
    bool operator<=(const CompositeKey& other) const { return !(other < *this); }
    bool operator>=(const CompositeKey& other) const { return !(*this < other); }
};

//make_key: создание ключа с втоматическим выводом типов
namespace detail {

    // Тип, в котором компонента хранится в ключе
    template<class T>
    struct KeyComponent {
        using type = T;
    };
    // оператор < дал бы сравнение адреса строк в памяти => явно указали string
    template<>
    struct KeyComponent<const char*> {
        using type = std::string;
    };

    template<>
    struct KeyComponent<char*> {
        using type = std::string;
    };

    template<class T>
    using KeyComponentT = typename KeyComponent<std::decay_t<T>>::type;
    // std::decay_t<T> очищает тип, как при передаче по значению (убирает const, &, превращает массив в указатель)
}

template<class... Ts>
CompositeKey<detail::KeyComponentT<Ts>...> make_key(const Ts&... values) {
    return CompositeKey<detail::KeyComponentT<Ts>...>(values...);
}
