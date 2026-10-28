#pragma once

#include "person.h"
#include <ostream>

// вывод в поток
inline std::ostream& operator<<(std::ostream& os, const Person& p) {
    os << "#" << p.get_id() << " " << p.get_full_name() << " (" << p.get_birth_year() << ")";
    return os;
}