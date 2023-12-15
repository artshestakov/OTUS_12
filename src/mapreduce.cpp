#include "mapreduce.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <sstream>
#include <iostream>
//-----------------------------------------------------------------------------
MapReduce::MapReduce(unsigned int m, unsigned int r)
    : m_Map(m),
    m_Reduce(r),
    m_ActiveThread(0)
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
bool MapReduce::Map(const std::string& file_path)
{
    if (!Split(file_path))
    {
        return false;
    }

    m_ActiveThread = (unsigned int)m_Chunks.size();
    for (std::string& chunk : m_Chunks)
    {
        std::thread(&MapReduce::Worker, this, std::ref(chunk)).detach();
    }

    //Ждём, пока все потоки завершат свою работу
    while (m_ActiveThread > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return true;
}
//-----------------------------------------------------------------------------
bool MapReduce::Reduce()
{
    return true;
}
//-----------------------------------------------------------------------------
bool MapReduce::Split(const std::string& file_path)
{
    std::error_code e;
    uintmax_t file_size = std::filesystem::file_size(file_path, e);
    if (e)
    {
        m_ErrorString = e.message();
        return false;
    }

    std::cout << "Start reading file " << file_path << std::endl;

    //Пытаемся открыть файл
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        m_ErrorString = strerror(errno);
        return false;
    }

    size_t chunk_size = (size_t)file_size / m_Map;
    size_t pos = 0;
    auto time_point = GetTick();

    std::vector<char> vec(chunk_size);
    while (file.read(&vec[0], chunk_size))
    {
        std::cout << "Reading " << std::to_string(m_Chunks.size() + 1) << " chunk..." << std::endl;

        //Поработаем с очередным прочитанным блоком данных
        ProcessVector(vec);

        pos += vec.size();
        file.seekg(pos);
    }

    //Последний блок данных
    ProcessVector(vec);

    std::cout << "Read OK by " << GetTickDiff(time_point) << " msec" << std::endl;
    return true;
}
//-----------------------------------------------------------------------------
void MapReduce::Worker(std::string& s)
{
    std::string thread_id = GetCurrentThreadID();
    printf("%s\tstarted thread\n", thread_id.c_str());

    //Заранее подсчитаем кол-во строк для вектора
    size_t reserve_size = 0;
    for (size_t i = 0, c = s.size(); i < c; ++i)
    {
        if (s[i] == '\n')
        {
            ++reserve_size;
        }
    }

    std::istringstream stream(s);
    std::vector<std::string> v;
    v.reserve(reserve_size);

    auto time_point = GetTick();

    std::string line;
    while (std::getline(stream, line))
    {
        v.emplace_back(std::move(line));
    }

    //Отдаём память обратно
    s.clear();
    s.shrink_to_fit();

    std::sort(v.begin(), v.end());

    //"Сигналим", что этот поток завершил работу и отдаём результат в список векторов
    m_Mutex.lock();
    --m_ActiveThread;
    m_VectorTotal.emplace_back(std::move(v));
    m_Mutex.unlock();

    printf("%s\tfinished thread by %llu msec\n", thread_id.c_str(), GetTickDiff(time_point));
}
//-----------------------------------------------------------------------------
std::string MapReduce::GetCurrentThreadID()
{
    std::ostringstream stream;
    stream << std::this_thread::get_id();
    return stream.str();
}
//-----------------------------------------------------------------------------
MapReduce::TimePoint MapReduce::GetTick()
{
    return std::chrono::steady_clock::now();
}
//-----------------------------------------------------------------------------
uint64_t MapReduce::GetTickDiff(const TimePoint& t)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(GetTick() - t).count();
}
//-----------------------------------------------------------------------------
void MapReduce::ProcessVector(std::vector<char>& vec)
{
    size_t erase_pos = 0;

    auto it_reverse = std::find(vec.rbegin(), vec.rend(), '\n');
    if (it_reverse == vec.rend())
    {
        auto it = std::find(vec.begin(), vec.end(), '\0');
        erase_pos = std::distance(vec.begin(), it);
    }
    else
    {
        erase_pos = std::distance(vec.begin(), it_reverse.base());
    }

    vec.erase(vec.begin() + erase_pos, vec.end());
    m_Chunks.emplace_back(std::move(std::string(vec.begin(), vec.end())));

    std::fill(vec.begin(), vec.end(), '\0');
}
//-----------------------------------------------------------------------------
