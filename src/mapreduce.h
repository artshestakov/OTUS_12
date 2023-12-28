#pragma once
//-----------------------------------------------------------------------------
#include <vector>
#include <mutex>
#include <unordered_map>
#include <filesystem>
//-----------------------------------------------------------------------------
class MapReduce
{
public:
    MapReduce(unsigned int m, unsigned int r, const std::string& file_path);
    ~MapReduce();

    const std::string& GetErrorString() const;
    size_t GetMinPrefix() const;

    bool Map();
    void Shuffle();
    bool Reduce();

private:
    bool Split();
    void WorkerMap(std::string& s);
    void WorkerReduce(const std::vector<std::string>& v, int file_index);
    void ProcessVector(std::vector<char>& vec);
    std::unordered_map<std::string, unsigned int> GetStringByMinSize(const std::vector<std::string>& v);
    bool PrepareOutputDir();

private:
    std::string m_ErrorString;
    unsigned int m_Map;
    unsigned int m_Reduce;
    std::string m_FilePath;
    std::filesystem::path m_DirOutput;
    std::vector<std::string> m_Chunks;
    unsigned int m_ActiveThread;
    std::mutex m_Mutex;
    std::vector<std::unordered_map<std::string, unsigned int>> m_VectorTotal;
    size_t m_MinPrefix;
};
//-----------------------------------------------------------------------------
