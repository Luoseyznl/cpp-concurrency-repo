cmake_minimum_required(VERSION 3.10)
project(02ThreadManagement)

set(CMAKE_CXX_STANDARD 20) # 支持 jthread
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制使用指定的 C++ 标准

include_directories(../utils)

# 1 基础线程管理
add_executable(02_basic 01_basic_management.cpp)

# 链接多线程库 (Linux下的基础依赖，Windows通常不需要)，标准库不规定用哪个线程库
find_package(Threads REQUIRED)
target_link_libraries(02_basic Threads::Threads)

# 2 现代 C++20 演示
add_executable(02_modern 02_modern_jthread.cpp)
target_link_libraries(02_modern Threads::Threads)
