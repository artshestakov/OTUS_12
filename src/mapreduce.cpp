#include "mapreduce.h"
#include <filesystem>
#include <fstream>
//-----------------------------------------------------------------------------
MapReduce::MapReduce()
{

}
//-----------------------------------------------------------------------------
MapReduce::~MapReduce()
{

}
//-----------------------------------------------------------------------------
const std::string& MapReduce::GetErrorString() const
{
    return m_ErrorString;
}
//-----------------------------------------------------------------------------
bool MapReduce::Split(const std::string& file_path, unsigned int m)
{
    std::error_code e;
    uintmax_t file_size = std::filesystem::file_size(file_path, e);
    if (e)
    {
        m_ErrorString = e.message();
        return false;
    }

    //�������� ������� ����
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        m_ErrorString = strerror(errno);
        return false;
    }

    uintmax_t chunk_size = file_size / m;
    size_t pos = 0;

    //������ ���� �� �������
    while (file_size > 0)
    {
        std::string tmp((size_t)chunk_size, '\0');
        file.read(&tmp[0], chunk_size);

        //���� ������ ����� ������ � �����. ��������� ������� ����� �������� ������ �����
        size_t border_pos = tmp.rfind('\n');
        if (border_pos == std::string::npos)
        {
            //���� ������ �� �����, ������ ���-�� �� ��� - ������� � �������
            m_ErrorString = "Could not find new line symbol";
            return false;
        }

        tmp.erase(++border_pos);
        m_Chunks.emplace_back(tmp);

        pos += border_pos;
        file.seekg(pos);

        file_size -= border_pos;
    }

    return true;
}
//-----------------------------------------------------------------------------
bool MapReduce::Run(const std::string& file_path, unsigned int m, unsigned int r)
{
    (void)file_path;
    (void)m;
    (void)r;
    return true;
}
//-----------------------------------------------------------------------------
