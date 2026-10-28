#pragma once

// Блок 9. Загрузка/сохранение коллекции Person в текстовый файл (CSV с разделителем ';').
// Нужен интерфейсу: "загрузить данные" и "сохранить данные".
// Формат (первая строка - заголовок):
//     id;last_name;first_name;middle_name;birth_year
//     1;Ivanov;Ivan;Ivanovich;1990
// Отчество может быть пустым: 2;Petrova;Anna;;1985

#include <fstream>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include "../model/person.h"
#include "../LR2/sequence.h"
#include "../LR2/mutableArraySequence.h"

struct PersonCsvResult {
    int loaded = 0;     // строк прочитано успешно
    int skipped = 0;    // строк с ошибкой (пропущены)
};

namespace detail {

    // Делит строку по ';' на ровно count полей; false - если полей другое число
    inline bool split_fields(const std::string& line, std::string* fields, int count) {
        int field = 0;
        fields[0].clear();
        for (std::size_t i = 0; i < line.size(); i++) {
            char c = line[i];
            if (c == '\r')
                continue;   // файл мог быть сохранён в Windows
            if (c == ';') {
                field++;
                if (field >= count)
                    return false;
                fields[field].clear();
            } else {
                fields[field] += c;
            }
        }
        return field == count - 1;
    }

    inline bool parse_int(const std::string& text, int& value) {
        try {
            std::size_t used = 0;
            value = std::stoi(text, &used);
            return used == text.size();
        } catch (const std::exception&) {
            return false;
        }
    }

}

inline void write_persons_csv(std::ostream& out, const Sequence<Person>& data) {
    out << "id;last_name;first_name;middle_name;birth_year\n";
    for (int i = 0; i < data.get_length(); i++) {
        const Person& p = data.get(i);
        out << p.get_id() << ';' << p.get_last_name() << ';' << p.get_first_name() << ';'
            << p.get_middle_name() << ';' << p.get_birth_year() << '\n';
    }
}

// Дописывает прочитанных людей в out. Плохие строки пропускаются и считаются в skipped.
inline PersonCsvResult read_persons_csv(std::istream& in, MutableArraySequence<Person>& out) {
    PersonCsvResult result;
    std::string line;
    bool first_line = true;

    while (std::getline(in, line)) {
        if (first_line) {
            first_line = false;
            if (line.rfind("id;", 0) == 0)
                continue;   // заголовок
        }
        if (line.empty() || line == "\r")
            continue;

        std::string fields[5];
        int id = 0;
        int year = 0;
        if (!detail::split_fields(line, fields, 5) || !detail::parse_int(fields[0], id)
            || !detail::parse_int(fields[4], year)) {
            result.skipped++;
            continue;
        }

        try {
            out.append(Person(id, fields[1], fields[2], fields[3], year));
            result.loaded++;
        } catch (const std::invalid_argument&) {   // пустое имя, год вне диапазона
            result.skipped++;
        }
    }
    return result;
}

inline bool save_persons_csv(const std::string& path, const Sequence<Person>& data) {
    std::ofstream file(path);
    if (!file)
        return false;
    write_persons_csv(file, data);
    return static_cast<bool>(file);
}

// false - файл не открылся
inline bool load_persons_csv(const std::string& path, MutableArraySequence<Person>& out,
                             PersonCsvResult& result) {
    std::ifstream file(path);
    if (!file)
        return false;
    result = read_persons_csv(file, out);
    return true;
}
