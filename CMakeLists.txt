cmake_minimum_required(VERSION 3.16)
project(laba2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Ядро: header-only шаблоны (Sequence, деревья, хеш-таблица, индексы)
add_library(laba2_core INTERFACE
        include/model/person.h)
target_include_directories(laba2_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Основная программа (UI)
add_executable(laba2 src/main.cpp
        include/model/person.h
        include/model/person_generator.h
        include/common/array_algorithms.h
        include/dictionary/i_dictionary.h
        include/common/pair.h
        include/common/pair_print.h
        include/common/exception.h
        include/dictionary/i_iterator.h
        include/dictionary/sequence_dictionary.h
        include/hash/hash_functions.h
        include/dictionary/hash_table.h)
target_link_libraries(laba2 PRIVATE laba2_core)

# Тесты
enable_testing()
add_executable(laba2_tests tests/tests_main.cpp
        include/model/person.h)
target_link_libraries(laba2_tests PRIVATE laba2_core)
add_test(NAME laba2_tests COMMAND laba2_tests)
