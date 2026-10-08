#ifndef LABA2_SEQUENCE_PRINT_H
#define LABA2_SEQUENCE_PRINT_H

#include "sequence.h"
#include <ostream>

template<class T>
std::ostream& operator<<(std::ostream &os, const Sequence<T> &seq) {
    auto it = seq.get_enumerator();

    os << "[";

    while (it->has_more_elements()) {
        os << it->next();

        if (it->has_more_elements())
            os << ", ";
    }

    os << "]";

    delete it;
    return os;
}

#endif //LABA2_SEQUENCE_PRINT_H
