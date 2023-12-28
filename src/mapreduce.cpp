#include "mapreduce.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <algorithm>
//-----------------------------------------------------------------------------
MapReduce::MapReduce(unsigned int m, unsigned int r, const std::string& file_path)
    : m_Map(m),
    m_Reduce(r),
    m_FilePath(file_path),
    m_DirOutput(std::filesystem::path(file_path).parent_path() / "output"),
    m_ActiveThread(0),
    m_MinPrefix(0)
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
size_t MapReduce::GetMinPrefix() const
{
    return m_MinPrefix;
}
//-----------------------------------------------------------------------------
bool MapReduce::Map()
{
    if (!Split())
    {
        return false;
    }

    m_ActiveThread = (unsigned int)m_Chunks.size();
    for (std::string& chunk : m_Chunks)
    {
        std::thread(&MapReduce::WorkerMap, this, std::ref(chunk)).detach();
    }

    //Ждём, пока все потоки завершат свою работу
    while (m_ActiveThread > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return true;
}
//-----------------------------------------------------------------------------
void MapReduce::Shuffle()
{
    //Если элементов больше одного, тогда переместим все в первый
    if (m_VectorTotal.size() > 1)
    {
        //Берём ссылку на первый элемент
        auto& first_map = m_VectorTotal.front();

        for (size_t i = 1; i < m_VectorTotal.size(); ++i)
        {
            auto& current_map = m_VectorTotal[i];
            first_map.insert(current_map.begin(), current_map.end());
            current_map.clear(); //Осводим память
        }

        //И грохнем все, кроме первого элемента
        m_VectorTotal.resize(1);
    }
}
//-----------------------------------------------------------------------------
bool MapReduce::Reduce()
{
    if (!PrepareOutputDir())
    {
        return false;
    }

    //Берём ссылку на уже подготовленный элемент
    auto& first_map = m_VectorTotal.front();

    //Сформируем вектор, для передачи его в поток-редьюса
    std::vector<std::string> v;
    v.resize(first_map.size());

    int i = -1; //Так и должно быть, чтобы ниже использовать ++i, а не i++
    for (const auto& a : first_map)
    {
        v[++i] = a.first;
    }

    unsigned int v_size = v.size();
    unsigned int chunk_size = v_size / m_Reduce;
    unsigned int offset = 0;
    m_ActiveThread = m_Reduce + (v_size % m_Reduce > 0 ? 1 : 0); //Расчёт кол-ва потоков
    int file_index = 0;

    while (v_size > 0)
    {
        if (v_size < chunk_size)
        {
            chunk_size = v_size;
        }

        std::thread(&MapReduce::WorkerReduce, this,
            std::vector<std::string>(v.begin() + offset, v.begin() + chunk_size + offset),
            ++file_index).detach();

        v_size -= chunk_size;
        offset += chunk_size;
    }


    //Ждём, пока все потоки завершат свою работу
    while (m_ActiveThread > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return true;
}
//-----------------------------------------------------------------------------
bool MapReduce::Split()
{
    std::error_code e;
    uintmax_t file_size = std::filesystem::file_size(m_FilePath, e);
    if (e)
    {
        m_ErrorString = e.message();
        return false;
    }

    std::cout << "Start reading file " << m_FilePath << std::endl;

    //Пытаемся открыть файл
    std::ifstream file(m_FilePath);
    if (!file.is_open())
    {
        m_ErrorString = strerror(errno);
        return false;
    }

    //Подсчитываем кол-во частей, на которые будет подёлен файл
    size_t chunk_size = (size_t)file_size / m_Map;
    size_t pos = 0;
    auto time_point = utils::GetTick();

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

    std::cout << "Read OK by " << utils::GetTickDiff(time_point) << " msec" << std::endl;
    return true;
}
//-----------------------------------------------------------------------------
void MapReduce::WorkerMap(std::string& s)
{
    std::string thread_id = utils::GetCurrentThreadID();
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

    auto time_point = utils::GetTick();

    //Сформируем вектор строк
    std::string line;
    while (std::getline(stream, line))
    {
        //Помимо формирования вектора со строками, мы ещё и найдём размер самой маленькой строки
        //Это нам потребуется для поиска префикса

        auto line_size = line.size();
        if (line_size == 0)
        {
            //Пропустим пустые строки. Зачем они нам?
            continue;
        }

        if (m_MinPrefix == 0 || line_size < m_MinPrefix)
        {
            m_MinPrefix = line_size;
        }

        v.emplace_back(std::move(line));
    }

    //Отдаём память обратно
    s.clear();
    s.shrink_to_fit();

    auto strings = GetStringByMinSize(v);

    //"Сигналим", что этот поток завершил работу и отдаём результат в список векторов
    m_Mutex.lock();
    m_VectorTotal.emplace_back(strings);
    --m_ActiveThread;
    m_Mutex.unlock();

    printf("%s\tfinished thread by %llu msec\n", thread_id.c_str(), utils::GetTickDiff(time_point));
}
//-----------------------------------------------------------------------------
void MapReduce::WorkerReduce(const std::vector<std::string>& v, int file_index)
{
    std::string file_name = "file" + std::to_string(file_index) + ".txt";

    std::ofstream file_output(m_DirOutput / file_name);
    for (const auto& str : v)
    {
        file_output << str << std::endl;
    }

    //"Сигналим", что этот поток завершил работу и отдаём результат в список векторов
    m_Mutex.lock();
    --m_ActiveThread;
    m_Mutex.unlock();
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

    //Нужно учитывать, что позиция должна быть отлична от нуля
    //Такое может быть, например, когда мы читаем файл в один поток мапперов и по факту файл читается целиком
    if (erase_pos > 0)
    {
        vec.erase(vec.begin() + erase_pos, vec.end());
        m_Chunks.emplace_back(std::move(std::string(vec.begin(), vec.end())));

        std::fill(vec.begin(), vec.end(), '\0');
    }
}
//-----------------------------------------------------------------------------
std::unordered_map<std::string, unsigned int> MapReduce::GetStringByMinSize(const std::vector<std::string>& v)
{
    std::unordered_map<std::string, unsigned int> m;

    for (const std::string& str : v)
    {
        std::string part_of_string = str.substr(0, m_MinPrefix);

        auto it = m.find(part_of_string);
        if (it != m.end())
        {
            ++m[part_of_string];
        }
        else
        {
            m[part_of_string] = 1;
        }
    }

    return m;
}
//-----------------------------------------------------------------------------
bool MapReduce::PrepareOutputDir()
{
    std::error_code e;

    //Проверим, не существует ли директория
    bool dir_exists = std::filesystem::exists(m_DirOutput, e);
    if (e)
    {
        m_ErrorString = e.message();
        return false;
    }

    //Если существует - удаляем её и все что в ней есть (чтобы не заморачиваться)
    if (dir_exists)
    {
        (void)std::filesystem::remove_all(m_DirOutput, e);
        if (e)
        {
            m_ErrorString = e.message();
            return false;
        }
    }

    //Создаём её по-новой
    (void)std::filesystem::create_directory(m_DirOutput, e);
    if (e)
    {
        m_ErrorString = e.message();
        return false;
    }

    return true;
}
//-----------------------------------------------------------------------------
