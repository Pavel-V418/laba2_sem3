#pragma once

// Блок 9. Ввод с клавиатуры с проверкой (как ReadInt в ЛР-1).
// Ввод построчный: строку читаем целиком и разбираем, поэтому числа и строки
// с пробелами ("Ivanov Ivan Ivanovich") можно смешивать без cin.ignore.

#include <iostream>
#include <stdexcept>
#include <string>

// Ввод закончился (Ctrl+Z / Ctrl+D или конец файла при запуске со сценарием)
class InputClosedException : public std::runtime_error
{
public:
    InputClosedException()
        : std::runtime_error("input closed") {}
};

inline std::string ReadLine()
{
    std::string line;
    if (!std::getline(std::cin, line))
        throw InputClosedException();

    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    return line;
}

// Убирает пробелы по краям
inline std::string Trim(const std::string& text)
{
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && (text[begin] == ' ' || text[begin] == '\t'))
        begin++;
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t'))
        end--;
    return text.substr(begin, end - begin);
}

inline bool TryParseInt(const std::string& text, int& value)
{
    try
    {
        std::size_t used = 0;
        value = std::stoi(text, &used);
        return used == text.size();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

inline int ReadInt()
{
    while (true)
    {
        int value;
        if (TryParseInt(Trim(ReadLine()), value))
            return value;

        std::cout << "Invalid input. Enter number: ";
    }
}

inline int ReadIntInRange(int low, int high)
{
    while (true)
    {
        int value = ReadInt();
        if (value >= low && value <= high)
            return value;

        std::cout << "Enter number from " << low << " to " << high << ": ";
    }
}

// Пустая строка -> значение по умолчанию
inline int ReadIntOrDefault(int default_value)
{
    while (true)
    {
        std::string line = Trim(ReadLine());
        if (line.empty())
            return default_value;

        int value;
        if (TryParseInt(line, value))
            return value;

        std::cout << "Invalid input. Enter number (or empty for " << default_value << "): ";
    }
}

inline std::string ReadNonEmptyLine()
{
    while (true)
    {
        std::string line = Trim(ReadLine());
        if (!line.empty())
            return line;

        std::cout << "Value must not be empty: ";
    }
}

inline bool ReadYesNo()
{
    while (true)
    {
        std::string line = Trim(ReadLine());
        if (line == "y" || line == "Y")
            return true;
        if (line == "n" || line == "N")
            return false;

        std::cout << "Enter y or n: ";
    }
}
