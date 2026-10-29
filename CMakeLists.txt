cmake_minimum_required(VERSION 3.24)
project(laba2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Предупреждения - только для нашего кода (не для GoogleTest)
function(laba2_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
    endif()
endfunction()

# Ядро: header-only шаблоны (Sequence, деревья, хеш-таблица, индексы, бенчмарк)
add_library(laba2_core INTERFACE)
target_include_directories(laba2_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Основная программа (консольный интерфейс, блок 9)
add_executable(laba2
        src/main.cpp
        src/ui/console_input.h
        src/ui/app_state.h
        src/ui/data_menu.h
        src/ui/index_menu.h
        src/ui/search_menu.h
        src/ui/benchmark_menu.h
        include/model/person_print.h)
target_include_directories(laba2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(laba2 PRIVATE laba2_core)
laba2_warnings(laba2)

# Тесты на GoogleTest (блок 8)
option(LABA2_BUILD_TESTS "Build GoogleTest tests" ON)

if (LABA2_BUILD_TESTS)
    include(FetchContent)

    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)   # для MSVC
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(laba2_tests
            tests/test_helpers.h
            # блок 0
            tests/person_test.cpp          # блок 1
            tests/dictionary_test.cpp      # блок 2
            tests/hash_table_test.cpp      # блок 3
            tests/tree_test.cpp            # блок 4
            tests/composite_key_test.cpp   # блок 5
            tests/index_test.cpp           # блок 6
            # блок 7
    )     # блок 9 (файлы)
    target_include_directories(laba2_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(laba2_tests PRIVATE laba2_core GTest::gtest_main)
    laba2_warnings(laba2_tests)

    include(GoogleTest)
    gtest_discover_tests(laba2_tests DISCOVERY_TIMEOUT 60)
endif()
