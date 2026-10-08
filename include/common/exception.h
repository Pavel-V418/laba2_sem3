#pragma once

#include <stdexcept>
#include <string>

// Ключ не найден в словаре (get / remove по несуществующему ключу)
class KeyNotFoundException : public std::out_of_range {
public:
    explicit KeyNotFoundException(const std::string& message = "Key not found")
        : std::out_of_range(message) {}
};

// Попытка добавить ключ, который уже есть в словаре (add)
class DuplicateKeyException : public std::invalid_argument {
public:
    explicit DuplicateKeyException(const std::string& message = "Key already exists")
        : std::invalid_argument(message) {}
};

// Обращение к итератору, который вышел за границы коллекции
class IteratorOutOfRangeException : public std::out_of_range {
public:
    explicit IteratorOutOfRangeException(const std::string& message = "Iterator is out of range")
        : std::out_of_range(message) {}
};