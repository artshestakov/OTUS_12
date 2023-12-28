#pragma once
//-----------------------------------------------------------------------------
#include <chrono>
#include <string>
#include <sstream>
#include <thread>
//-----------------------------------------------------------------------------
namespace utils
{
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

    TimePoint GetTick();
    uint64_t GetTickDiff(const TimePoint& t);
    std::string GetCurrentThreadID();
}
//-----------------------------------------------------------------------------
