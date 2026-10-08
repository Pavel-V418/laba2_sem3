#pragma once

#include <string>
#include <cstring>   // memcpy
#include "../common/pair.h"

using HashCode = unsigned int;

// Финализатор MurmurHash3: "перемешивает" биты, чтобы близкие хеши (1, 2, 3...)
// попадали в далёкие друг от друга корзины.
inline HashCode mix_hash(HashCode h) {
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

// Комбинирование двух хешей (схема из boost::hash_combine).
// Порядок важен: combine(a, b) != combine(b, a), поэтому (1, 2) и (2, 1) получат разные хеши.
inline HashCode hash_combine(HashCode seed, HashCode value) {
    return seed ^ (value + 0x9e3779b9u + (seed << 6) + (seed >> 2));
}

// Общего шаблона нет специально: для неподдерживаемого типа будет ошибка компиляции,
// а не молча плохой хеш.
template<class T>
struct Hasher;

/*============ Целые числа ============*/

template<>
struct Hasher<int> {
    HashCode operator()(int key) const {
        return static_cast<HashCode>(key); // отрицательные числа тоже различаются
    }
};

template<>
struct Hasher<unsigned int> {
    HashCode operator()(unsigned int key) const {
        return key;
    }
};

template<>
struct Hasher<long long> {
    HashCode operator()(long long key) const {
        unsigned long long k = static_cast<unsigned long long>(key);
        return static_cast<HashCode>(k ^ (k >> 32)); // учитываем и старшие, и младшие 32 бита
    }
};

template<>
struct Hasher<char> {
    HashCode operator()(char key) const {
        return static_cast<HashCode>(static_cast<unsigned char>(key));
    }
};

/*============ Вещественные числа ============*/

template<>
struct Hasher<double> {
    HashCode operator()(double key) const {
        if (key == 0.0)
            return 0; // 0.0 == -0.0, но биты у них разные - иначе нарушится обязательное условие

        unsigned long long bits;
        std::memcpy(&bits, &key, sizeof(bits));
        return static_cast<HashCode>(bits ^ (bits >> 32));
    }
};

/*============ Строки ============*/

// FNV-1a: простой и хорошо распределяющий хеш для строк
template<>
struct Hasher<std::string> {
    HashCode operator()(const std::string& key) const {
        HashCode h = 2166136261u;           // offset basis
        for (std::size_t i = 0; i < key.size(); i++) {
            h ^= static_cast<unsigned char>(key[i]);
            h *= 16777619u;                 // FNV prime
        }
        return h;
    }
};

/*============ Кортежи ============*/

// Пара (например, (фамилия, год рождения) или (i, j) у разреженной матрицы).
// Хешируем каждую компоненту своим хешером и комбинируем.
template<class A, class B>
struct Hasher<Pair<A, B>> {
    HashCode operator()(const Pair<A, B>& p) const {
        HashCode h = Hasher<A>()(p.key);
        h = hash_combine(h, Hasher<B>()(p.value));
        return h;
    }
};
