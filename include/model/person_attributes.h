#pragma once

// Блок 6. Каталог атрибутов Person, по которым можно строить индексы.
// ТЗ (п. 2.5): "структура обрабатываемых данных и перечень атрибутов, по которым возможно
// строить индексы, следует зафиксировать" - вот этот перечень.
//
// У атрибутов РАЗНЫЕ типы ключей (int, string, CompositeKey), а меню нужно
// перебирать их в цикле. Поэтому каталог устроен через "посетителя" (visitor):
//     visit_attribute(attr, year, [&](auto extractor) { ... });
// В каждой ветке switch посетителю передаётся своя лямбда-извлекатель, а тип ключа
// внутри посетителя выводится из неё: using Key = decltype(extractor(Person()));

#include <string>
#include <stdexcept>
#include "person.h"
#include "../common/composite_key.h"

enum class PersonAttribute {
    Id,
    BirthYear,
    Age,                 // возраст на заданный год - GetAge(year)
    LastName,
    FirstName,
    FullName,
    Fio,                 // "Фамилия И.О."
    FirstLastBirthYear,  // (FirstName, LastName, BirthYear) - пример составного атрибута из ТЗ
    FullNameAge,         // (FullName, Age(year))             - второй пример из ТЗ
    BirthYearLastName    // (BirthYear, LastName) - диапазон по годам, внутри года по фамилии
};

static constexpr int PERSON_ATTRIBUTE_COUNT = 10;

inline PersonAttribute person_attribute_at(int number) {
    if (number < 0 || number >= PERSON_ATTRIBUTE_COUNT)
        throw std::out_of_range("person_attribute_at: number out of range");
    return static_cast<PersonAttribute>(number);
}

inline const char* to_string(PersonAttribute attribute) {
    switch (attribute) {
        case PersonAttribute::Id:                 return "id";
        case PersonAttribute::BirthYear:          return "birth_year";
        case PersonAttribute::Age:                return "age";
        case PersonAttribute::LastName:           return "last_name";
        case PersonAttribute::FirstName:          return "first_name";
        case PersonAttribute::FullName:           return "full_name";
        case PersonAttribute::Fio:                return "fio";
        case PersonAttribute::FirstLastBirthYear: return "(first_name, last_name, birth_year)";
        case PersonAttribute::FullNameAge:        return "(full_name, age)";
        case PersonAttribute::BirthYearLastName:  return "(birth_year, last_name)";
    }
    return "unknown";
}

// Нужен ли атрибуту год, на который считается возраст
inline bool needs_year(PersonAttribute attribute) {
    return attribute == PersonAttribute::Age || attribute == PersonAttribute::FullNameAge;
}

// Названия компонент ключа - для ввода ключа с клавиатуры (по одному полю на компоненту)
inline int component_count(PersonAttribute attribute) {
    switch (attribute) {
        case PersonAttribute::FirstLastBirthYear: return 3;
        case PersonAttribute::FullNameAge:        return 2;
        case PersonAttribute::BirthYearLastName:  return 2;
        default:                                  return 1;
    }
}

inline const char* component_name(PersonAttribute attribute, int component) {
    if (component < 0 || component >= component_count(attribute))
        throw std::out_of_range("component_name: component out of range");

    switch (attribute) {
        case PersonAttribute::FirstLastBirthYear: {
            static const char* names[] = {"first name", "last name", "birth year"};
            return names[component];
        }
        case PersonAttribute::FullNameAge: {
            static const char* names[] = {"full name", "age"};
            return names[component];
        }
        case PersonAttribute::BirthYearLastName: {
            static const char* names[] = {"birth year", "last name"};
            return names[component];
        }
        default:
            return to_string(attribute);
    }
}

// Здесь собраны готовые лямбды функции извлечения атрибута
template<class Visitor>
void visit_attribute(PersonAttribute attribute, int year, Visitor&& visitor) {
    switch (attribute) {
        case PersonAttribute::Id:
            visitor([](const Person& p) { return p.get_id(); });
            return;
        case PersonAttribute::BirthYear:
            visitor([](const Person& p) { return p.get_birth_year(); });
            return;
        case PersonAttribute::Age:
            visitor([year](const Person& p) { return p.get_age(year); });
            return;
        case PersonAttribute::LastName:
            visitor([](const Person& p) { return p.get_last_name(); });
            return;
        case PersonAttribute::FirstName:
            visitor([](const Person& p) { return p.get_first_name(); });
            return;
        case PersonAttribute::FullName:
            visitor([](const Person& p) { return p.get_full_name(); });
            return;
        case PersonAttribute::Fio:
            visitor([](const Person& p) { return p.get_fio(); });
            return;
        case PersonAttribute::FirstLastBirthYear:
            visitor([](const Person& p) {
                return make_key(p.get_first_name(), p.get_last_name(), p.get_birth_year());
            });
            return;
        case PersonAttribute::FullNameAge:
            visitor([year](const Person& p) { return make_key(p.get_full_name(), p.get_age(year)); });
            return;
        case PersonAttribute::BirthYearLastName:
            visitor([](const Person& p) { return make_key(p.get_birth_year(), p.get_last_name()); });
            return;
    }
    throw std::invalid_argument("visit_attribute: unknown attribute");
}
