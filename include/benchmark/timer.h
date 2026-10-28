#pragma once

#include <chrono>

// Блок 7. Секундомер - "функция измерения времени выполнения алгоритма" (раздел 1 ТЗ).
// steady_clock, а не system_clock: он монотонный и не прыгает при переводе системных часов.
class Timer {

public:
    Timer()
        : start_(Clock::now()) {}

    void restart() {
        start_ = Clock::now();
    }

    double elapsed_ms() const {
        return std::chrono::duration<double, std::milli>(Clock::now() - start_).count(); // duration переводит размерность в мс
    }

    double elapsed_us() const {
        return std::chrono::duration<double, std::micro>(Clock::now() - start_).count();
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point start_;
};
