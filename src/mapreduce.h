#pragma once
//-----------------------------------------------------------------------------
#include <string>
#include <vector>
#include <mutex>
//-----------------------------------------------------------------------------
class MapReduce
{
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

public:
    MapReduce(unsigned int m, unsigned int r);
    ~MapReduce();

    const std::string& GetErrorString() const;

    bool Map(const std::string& file_path);
    void Shuffle();
    bool Reduce();

private:
    bool Split(const std::string& file_path);
    void Worker(std::string& s);
    std::string GetCurrentThreadID();
    MapReduce::TimePoint GetTick();
    uint64_t GetTickDiff(const TimePoint& t);
    void ProcessVector(std::vector<char>& vec);

private:
    std::string m_ErrorString;
    unsigned int m_Map;
    unsigned int m_Reduce;
    std::vector<std::string> m_Chunks;
    unsigned int m_ActiveThread;
    std::mutex m_Mutex;
    std::vector<std::vector<std::string>> m_VectorTotal;
};
//-----------------------------------------------------------------------------
