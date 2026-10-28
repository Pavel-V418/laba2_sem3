#pragma once

#include <ostream>
#include "composite_key.h"

// Вывод составного ключа: (1990, Ivanov, 17)

namespace detail {

    inline void print_key_components(std::ostream&, const CompositeKey<>&) {}

    // печатает ", a, b, c" - каждая компонента с разделителем перед ней
    template<class Head, class... Tail>
    void print_key_components(std::ostream& os, const CompositeKey<Head, Tail...>& key) {
        os << ", " << key.head;
        print_key_components(os, key.tail);
    }

}

inline std::ostream& operator<<(std::ostream& os, const CompositeKey<>&) {
    return os << "()";
}

template<class Head, class... Tail>
std::ostream& operator<<(std::ostream& os, const CompositeKey<Head, Tail...>& key) {
    os << "(" << key.head;                       // первая компонента без запятой
    detail::print_key_components(os, key.tail);  // остальные через ", "
    os << ")";
    return os;
}