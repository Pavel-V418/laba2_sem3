#pragma once

// Извлекатель ключа: "по какому атрибуту строим индекс"
// Базовый класс: знает только "из TItem получить TKey". Про лямбду - ни слова
template<class TItem, class TKey>
class IKeyExtractor {
public:
    virtual ~IKeyExtractor() = default;

    virtual TKey extract(const TItem& item) const = 0;
};

// Наследник: для каждого типа лямбды F компилятор создаёт свою версию.
template<class TItem, class TKey, class F>
class FunctionKeyExtractor : public IKeyExtractor<TItem, TKey> {
public:
    explicit FunctionKeyExtractor(F function)
        : function_(function) {}

    TKey extract(const TItem& item) const override {
        return function_(item);
    }

private:
    F function_;
};

