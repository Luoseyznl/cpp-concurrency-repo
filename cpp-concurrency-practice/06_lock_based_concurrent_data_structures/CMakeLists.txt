cmake_minimum_required(VERSION 3.10)
project(06LockBasedConcurrentDataStructures)

set(CMAKE_CXX_STANDARD 17) # 需要 C++17 支持 std::shared_mutex
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

macro(add_ds_example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Threads::Threads)
endmacro()

add_ds_example(01_thread_safe_stack)
add_ds_example(02_thread_safe_queue)
add_ds_example(03_fine_grained_queue)
add_ds_example(04_lookup_table)
