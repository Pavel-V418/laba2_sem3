#pragma once

// Компаратор по умолчанию для деревьев поиска

template<class T>
struct KeyLess {
    bool operator()(const T& a, const T& b) const {
        return a < b;
    }
};

// Обратный порядок (например, индекс "сначала самые молодые") - дерево обходится по убыванию
template<class T>
struct KeyGreater {
    bool operator()(const T& a, const T& b) const {
        return b < a;
    }
};
