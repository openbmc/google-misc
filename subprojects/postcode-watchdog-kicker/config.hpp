#pragma once

#include <cstdint>
#include <vector>

/*
 * Each of these elements defines a non-default TimeRemaining
 * to use if that code is seen.  For instance, if you see the PoST code 0xae,
 * maybe you'll want to wait longer so that the kernel has time to come up
 * and take over.
 */
struct PostConfig
{
    uint8_t code;
    uint64_t value;
};

extern std::vector<struct PostConfig> IntervalOverrideConfig;
