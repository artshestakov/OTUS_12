#pragma once
//-----------------------------------------------------------------------------
#include <string>
#include <vector>
//-----------------------------------------------------------------------------
class MapReduce
{
public:
    MapReduce();
    ~MapReduce();

    const std::string& GetErrorString() const;

    bool Split(const std::string& file_path, unsigned int m);
    bool Run(const std::string &file_path, unsigned int m, unsigned int r);

private:
    std::string m_ErrorString;
    std::vector<std::string> m_Chunks;
};
//-----------------------------------------------------------------------------
