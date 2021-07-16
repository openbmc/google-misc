#pragma once
#include <fmt/format.h>

namespace flasher
{

enum class LogLevel : uint8_t
{
    Emerg = 0,
    Alet = 1,
    Crit = 2,
    Error = 3,
    Warning = 4,
    Notice = 5,
    Info = 6,
    Debug = 7,
};

extern LogLevel logLevel;

#define log(level, ...)                                                        \
    switch (0)                                                                 \
    default:                                                                   \
        if ((level > ::flasher::logLevel))                                  \
        {                                                                      \
        }                                                                      \
        else                                                                   \
            ::fmt::print(stderr, __VA_ARGS__)

} // namespace flasher
