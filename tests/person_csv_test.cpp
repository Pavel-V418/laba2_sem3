// Блок 9: загрузка/сохранение коллекции в файл (используется интерфейсом)

#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "io/person_csv.h"
#include "model/person_generator.h"
#include "model/person_print.h"

TEST(PersonCsv, RoundTrip) {
    PersonGenerator generator(12);
    PersonSequencePtr people = generator.generate(200);

    std::stringstream buffer;
    write_persons_csv(buffer, *people);

    MutableArraySequence<Person> loaded;
    PersonCsvResult result = read_persons_csv(buffer, loaded);

    EXPECT_EQ(result.loaded, 200);
    EXPECT_EQ(result.skipped, 0);
    ASSERT_EQ(loaded.get_length(), 200);
    for (int i = 0; i < 200; i++)
        EXPECT_EQ(loaded.get(i), people->get(i));
}

TEST(PersonCsv, EmptyMiddleNameAndWindowsLineEnds) {
    std::istringstream in("id;last_name;first_name;middle_name;birth_year\r\n"
                          "2;Petrova;Anna;;1985\r\n");
    MutableArraySequence<Person> loaded;
    PersonCsvResult result = read_persons_csv(in, loaded);

    ASSERT_EQ(result.loaded, 1);
    EXPECT_EQ(loaded.get(0), Person(2, "Petrova", "Anna", "", 1985));
}

TEST(PersonCsv, BadLinesAreSkipped) {
    std::istringstream in("id;last_name;first_name;middle_name;birth_year\n"
                          "1;Ivanov;Ivan;Ivanovich;1990\n"
                          "x;Bad;Id;;1990\n"            // id не число
                          "3;Short;Line\n"              // мало полей
                          "4;;Noname;;1990\n"           // пустая фамилия
                          "5;Old;Man;;1500\n"           // год вне диапазона
                          "6;Too;Many;Fields;1990;7\n"  // лишнее поле
                          "\n"                          // пустая строка - не ошибка
                          "7;Petrov;Petr;;2001\n");
    MutableArraySequence<Person> loaded;
    PersonCsvResult result = read_persons_csv(in, loaded);

    EXPECT_EQ(result.loaded, 2);
    EXPECT_EQ(result.skipped, 5);
    EXPECT_EQ(loaded.get(1).get_id(), 7);
}

TEST(PersonCsv, MissingFile) {
    MutableArraySequence<Person> loaded;
    PersonCsvResult result;
    EXPECT_FALSE(load_persons_csv("no/such/dir/file.csv", loaded, result));
}
