#pragma once

template<class T>
class IIterator {

public:
    virtual~IIterator() = default;

    virtual bool has_next() const = 0;
    virtual bool next() = 0;
    virtual const T& get_current_item() const = 0;

    // Не виртуальный: выражается через has_next и get_current_item,
    // поэтому одинаков для всех итераторов
    bool try_get_current_item(T& element) const {
        if (!has_next())
            return false;
        element = get_current_item();
        return true;
    }

};