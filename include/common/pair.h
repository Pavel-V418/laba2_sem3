#pragma once

template<class TKey, class TValue>
struct Pair {
    TKey key;
    TValue value;

    Pair()
        :key(), value() {}
    Pair(const TKey& key, const TValue& value)
        : key(key), value(value) {}

    bool operator==(const Pair& other) const {
        return key == other.key && value == other.value;
    }

    bool operator!=(const Pair& other) const {
        return !(*this == other);
    }

};