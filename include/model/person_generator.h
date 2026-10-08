#pragma once

#include <random>
#include <stdexcept>
#include <memory>
#include "model/person.h"
#include "../common/array_algorithms.h"
#include "../LR2//mutableArraySequence.h"

// Порядок входных данных - три обязательных случая для сравнения алгоритмов
enum class DataOrder {
    Random,     // не отсортирована
    Sorted,     // отсортирована в нужном направлении
    Reversed    // отсортирована в обратном направлении
};


inline const char* to_string(DataOrder order) {
    switch (order) {
        case DataOrder::Random:
            return "random";
        case DataOrder::Sorted:
            return "sorted";
        case DataOrder::Reversed:
            return "reversed";
    }
    return "unknown";
}

// Владеющий указатель на сгенерированную коллекцию: память освобождается сама
using PersonSequencePtr = std::unique_ptr<MutableArraySequence<Person>>;

// Генератор создаёт тестовые коллекции людей нужного размера и в нужном порядке.
class PersonGenerator {

public:
    // Один и тот же seed - одни и те же данные: это нужно, чтобы сравнивать разные структуры на ОДИНАКОВЫХ входных данных.
    explicit PersonGenerator(unsigned seed = 42) : rng_(seed) {}

    // Случайный порядок
    PersonSequencePtr generate(int count) {
        DynamicArray<Person> arr = generate_array(count);
        return to_sequence(arr);
    }

    // Заданный порядок по критерию less (например, PersonByIdLess)
    template<class Less>
    PersonSequencePtr generate(int count, DataOrder order, Less less) {
        DynamicArray<Person> arr = generate_array(count);

        if (order != DataOrder::Random) {
            merge_sort(arr, less);
            if (order == DataOrder::Reversed)
                reverse(arr); // перевернули
        }

        return to_sequence(arr);
    }

    // Один случайный человек с заданным id
    Person generate_one(int id) {
        bool male = random_int(0, 1) == 0;

        std::string last = SURNAMES[random_int(0, SURNAMES_COUNT - 1)];
        if (!male)
            last += "a";   // Ivanov -> Ivanova (все фамилии в списке на -ov/-ev/-in)

        // ПЕРЕПИСАТЬ ЧЕРЕЗ if!!!
        std::string first = male
            ? MALE_NAMES[random_int(0, MALE_NAMES_COUNT - 1)]
            : FEMALE_NAMES[random_int(0, FEMALE_NAMES_COUNT - 1)];

        int p = random_int(0, PATRONYMICS_COUNT - 1); // p — это просто номер отчества в списке, случайное число от 0 до 14
        std::string middle = male ? MALE_PATRONYMICS[p] : FEMALE_PATRONYMICS[p];

        int year = random_int(MIN_YEAR, MAX_YEAR);

        return Person(id, last, first, middle, year);
    }

    static constexpr int MIN_YEAR = 1940;
    static constexpr int MAX_YEAR = 2010;

private:
    std::mt19937 rng_; // генератор псевдослучайных чисел

    int random_int(int low, int high) {
        std::uniform_int_distribution<int> dist(low, high);
        return dist(rng_);
    } // случайное число в диапозоне

    // создание массива из count случайных людей с id от 1 до count + shuffle
    DynamicArray<Person> generate_array(int count) {
        if (count < 0)
            throw std::invalid_argument("PersonGenerator: count must be non-negative");

        DynamicArray<Person> arr(count);
        for (int i = 0; i < count; i++)
            arr.set(generate_one(i + 1), i);

        shuffle(arr, rng_); // перемешали
        return arr;
    }

    static PersonSequencePtr to_sequence(const DynamicArray<Person>& arr) {
        PersonSequencePtr seq = std::make_unique<MutableArraySequence<Person>>(); // что делает
        for (int i = 0; i < arr.get_size(); i++)
            seq->append(arr.get(i));
        return seq;
    }

    // Справочники имён
    static constexpr const char* SURNAMES[] = {
        "Ivanov", "Smirnov", "Kuznetsov", "Popov", "Vasilyev", "Petrov", "Sokolov",
        "Mikhailov", "Novikov", "Fedorov", "Morozov", "Volkov", "Alekseev", "Lebedev",
        "Semenov", "Egorov", "Pavlov", "Kozlov", "Stepanov", "Nikolaev", "Orlov",
        "Andreev", "Makarov", "Nikitin", "Zakharov", "Zaytsev", "Solovyov", "Borisov",
        "Yakovlev", "Grigoryev", "Romanov", "Vorobyov", "Sergeev", "Frolov", "Belov",
        "Medvedev", "Antonov", "Tarasov", "Zhukov", "Baranov"
    };
    static constexpr int SURNAMES_COUNT = sizeof(SURNAMES) / sizeof(SURNAMES[0]);

    static constexpr const char* MALE_NAMES[] = {
        "Aleksandr", "Dmitry", "Maksim", "Sergey", "Andrey", "Aleksey", "Artem",
        "Ilya", "Kirill", "Mikhail", "Nikita", "Matvey", "Roman", "Egor", "Ivan",
        "Pavel", "Denis", "Timur", "Vladimir", "Konstantin"
    };
    static constexpr int MALE_NAMES_COUNT = sizeof(MALE_NAMES) / sizeof(MALE_NAMES[0]);

    static constexpr const char* FEMALE_NAMES[] = {
        "Anna", "Maria", "Elena", "Olga", "Natalia", "Tatiana", "Irina", "Ekaterina",
        "Svetlana", "Yulia", "Daria", "Polina", "Sofia", "Alisa", "Viktoria",
        "Ksenia", "Anastasia", "Valeria", "Veronika", "Alina"
    };
    static constexpr int FEMALE_NAMES_COUNT = sizeof(FEMALE_NAMES) / sizeof(FEMALE_NAMES[0]);

    // мужская и женская форма отчества стоят под одинаковыми индексами
    static constexpr const char* MALE_PATRONYMICS[] = {
        "Aleksandrovich", "Dmitrievich", "Sergeevich", "Andreevich", "Alekseevich",
        "Ivanovich", "Mikhailovich", "Pavlovich", "Vladimirovich", "Nikolaevich",
        "Petrovich", "Viktorovich", "Olegovich", "Igorevich", "Yuryevich"
    };
    static constexpr const char* FEMALE_PATRONYMICS[] = {
        "Aleksandrovna", "Dmitrievna", "Sergeevna", "Andreevna", "Alekseevna",
        "Ivanovna", "Mikhailovna", "Pavlovna", "Vladimirovna", "Nikolaevna",
        "Petrovna", "Viktorovna", "Olegovna", "Igorevna", "Yuryevna"
    };
    static constexpr int PATRONYMICS_COUNT = sizeof(MALE_PATRONYMICS) / sizeof(MALE_PATRONYMICS[0]);
};

