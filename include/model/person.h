#pragma once
#include <string>
#include <ostream>
#include <stdexcept>

// Информационный объект, по атрибутам которого строятся индексы.
// Требования DynamicArray: есть конструктор по умолчанию и копирующее присваивание.
class Person {

public:
    Person() : id_(0), birth_year_(0) {}

    Person(int id, const std::string& last_name, const std::string& first_name, const std::string& middle_name, int birth_year)
        : id_(id), last_name_(last_name), first_name_(first_name), middle_name_(middle_name), birth_year_(birth_year) {
        if (last_name.empty() || first_name.empty())
            throw std::invalid_argument("Person: last and first name must not be empty");
        if (birth_year < MIN_BIRTH_YEAR || birth_year > MAX_BIRTH_YEAR)
            throw std::invalid_argument("Person: birth year out of range");
    }

    // Геттеры
    int get_id() const { return id_; }
    const std::string& get_first_name() const { return first_name_; }
    const std::string& get_middle_name() const { return middle_name_; }
    const std::string& get_last_name() const { return last_name_; }
    int get_birth_year() const { return birth_year_; }

    // "Фамилия Имя Отчество"
    std::string get_full_name() const {
        std::string result = last_name_ + " " + first_name_;

        if (!middle_name_.empty())
            result += " " + middle_name_;

        return result;
    }

    // "Фамилия И.О."
    std::string get_fio() const {
        std::string result = last_name_;

        if (!first_name_.empty()) {
            result += " ";
            result += first_name_[0];
            result += ".";
        }

        if (!middle_name_.empty()) {
            result += middle_name_[0];
            result += ".";
        }

        return result;
    }

    // Возраст на заданный год
    int get_age(int year) const {
        if (year < birth_year_)
            throw std::invalid_argument("Person::get_age: year is before birth year");
        return year - birth_year_;
    }

    bool operator==(const Person& other) const {
        return id_ == other.id_
            && last_name_ == other.last_name_
            && first_name_ == other.first_name_
            && middle_name_ == other.middle_name_
            && birth_year_ == other.birth_year_;
    }

    bool operator!=(const Person& other) const { return !(*this == other); }

    static constexpr int MIN_BIRTH_YEAR = 1900;
    static constexpr int MAX_BIRTH_YEAR = 2100;

private:
    int id_;
    std::string last_name_;
    std::string first_name_;
    std::string middle_name_; // отчество м.б. пустым
    int birth_year_;
};

// вывод в поток
inline std::ostream& operator<<(std::ostream& os, const Person& p) {
    os << "#" << p.get_id() << " " << p.get_full_name() << " (" << p.get_birth_year() << ")";
    return os;
}

// Компараторы

struct PersonByIdLess {
    bool operator()(const Person& a, const Person& b) const {
        return a.get_id() < b.get_id();
    }
};

struct PersonByBirthYearLess {
    bool operator()(const Person& a, const Person& b) const {
        return a.get_birth_year() < b.get_birth_year();
    }
};

struct PersonByFullNameLess {
    bool operator()(const Person& a, const Person& b) const {
        return a.get_full_name() < b.get_full_name();
    }
};

