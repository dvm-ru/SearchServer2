#pragma once

#include <chrono>
#include <iostream>
#include <string_view>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)

/**
 * Макрос замеряет время, прошедшее с момента своего вызова
 * до конца текущего блока, и выводит в поток std::cerr.
 *
 * Пример использования:
 *
 *  void Task1() {
 *      LOG_DURATION("Task 1"s); // Выведет в cerr время работы функции Task1
 *      ...
 *  }
 *
 *  void Task2() {
 *      LOG_DURATION("Task 2"s); // Выведет в cerr время работы функции Task2
 *      ...
 *  }
 *
 *  int main() {
 *      LOG_DURATION("main"s);  // Выведет в cerr время работы функции main
 *      Task1();
 *      Task2();
 *  }
 */
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

 /**
  * Поведение аналогично макросу LOG_DURATION, при этом можно указать поток,
  * в который должно быть выведено измеренное время.
  *
  * Пример использования:
  *
  *  int main() {
  *      // Выведет время работы main в поток std::cout
  *      LOG_DURATION("main"s, std::cout);
  *      ...
  *  }
  */
#define LOG_DURATION_STREAM(x, y) LogDuration UNIQUE_VAR_NAME_PROFILE(x, y)

class LogDuration {
public:
    // ������� ��� ���� std::chrono::steady_clock
    // � ������� using ��� ��������
    using Clock = std::chrono::steady_clock;

    LogDuration(std::string_view id, std::ostream& dst_stream = std::cerr)
        : id_(id)
        , dst_stream_(dst_stream) {
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        dst_stream_ << id_ << ": "sv << duration_cast<milliseconds>(dur).count() << " ms"sv << std::endl;
    }

private:
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& dst_stream_;
};