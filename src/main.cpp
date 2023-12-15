#include <iostream>
#include <filesystem>
#include <thread>
#include "mapreduce.h"
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    auto print_error = []()
    {
        std::cout << "Invalid arguments!" << std::endl <<
            "  Example: mapreduce <src_file> <m_num> <r_num>" << std::endl <<
            "  OR" << std::endl <<
            "  mapreduce some_file.txt 5 5" << std::endl;
    };

    //��������, ��� ���������� ����������
    if (argc != 4)
    {
        print_error();
        return EXIT_FAILURE;
    }

    //��������, ��� ���� ������ ����������
    std::string file_path(argv[1]);
    if (!std::filesystem::exists(file_path))
    {
        std::cout << "File \"" << file_path << "\" is not exists" << std::endl;
        return EXIT_FAILURE;
    }

    unsigned int m = (unsigned int)std::atoi(argv[2]);
    unsigned int r = (unsigned int)std::atoi(argv[3]);
    auto max_thread_count = std::thread::hardware_concurrency() * 4;

    //��������, ��� �������� ��������� ������� ��������� � ��� �� ������� "����������� �����"
    if ((!m || !r) ||
        (m > max_thread_count || r > max_thread_count))
    {
        print_error();
        return EXIT_FAILURE;
    }

    MapReduce mr(m, r);

    if (!mr.Map(file_path))
    {
        std::cout << mr.GetErrorString() << std::endl;
        return EXIT_FAILURE;
    }

    if (!mr.Reduce())
    {
        std::cout << mr.GetErrorString() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
//-----------------------------------------------------------------------------
