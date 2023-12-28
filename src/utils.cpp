#include "utils.h"
//-----------------------------------------------------------------------------
utils::TimePoint utils::GetTick()
{
    return std::chrono::steady_clock::now();
}
//-----------------------------------------------------------------------------
uint64_t utils::GetTickDiff(const TimePoint& t)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(GetTick() - t).count();
}
//-----------------------------------------------------------------------------
std::string utils::GetCurrentThreadID()
{
    std::ostringstream stream;
    stream << std::this_thread::get_id();
    return stream.str();
}
//-----------------------------------------------------------------------------
