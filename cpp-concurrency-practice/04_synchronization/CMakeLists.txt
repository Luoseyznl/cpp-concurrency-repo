cmake_minimum_required(VERSION 3.10)
project(04SynchronizingOperations)

set(CMAKE_CXX_STANDARD 20) # 支持 C++20 特性 (latch/barrier 等)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制使用指定的 C++ 标准

find_package(Threads REQUIRED)
find_package(Boost COMPONENTS thread system)

macro(add_sync_example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Threads::Threads)
endmacro()

add_sync_example(01_condition_variable)
add_sync_example(02_async_deferred)
add_sync_example(03_packaged_task)
add_sync_example(04_promise_future)
add_sync_example(05_shared_future)
add_sync_example(06_chrono_utils)

if(Boost_FOUND)
    add_executable(07_boost_chaining 07_boost_chaining.cpp)
    target_link_libraries(07_boost_chaining PRIVATE Threads::Threads Boost::thread Boost::system)
    message(STATUS "Boost found. Building chain example.")
else()
    message(WARNING "Boost NOT found. Skipping 07_boost_chaining.")
endif()
