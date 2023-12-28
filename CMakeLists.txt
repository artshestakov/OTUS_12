cmake_minimum_required(VERSION 3.12)

project(mapreduce)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(WIN32)
    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS
    )
endif()

add_executable(mapreduce
    src/main.cpp
    src/mapreduce.cpp
    src/utils.cpp
)

target_link_libraries(mapreduce PRIVATE Threads::Threads)

install(TARGETS mapreduce RUNTIME DESTINATION bin)
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_FILE_NAME "mapreduce")
set(CPACK_PACKAGE_CONTACT art.shestakov@icloud.com)
include(CPack)
