cmake_minimum_required(VERSION 3.10)
project(03SharingData)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(01_thread_safe_stack 01_thread_safe_stack.cpp)
add_executable(02_deadlock_avoidance 02_deadlock_avoidance.cpp)
add_executable(03_lock_flexibility 03_lock_flexibility.cpp)
add_executable(04_call_once_singleton 04_call_once_singleton.cpp)
add_executable(05_shared_mutex_dns 05_shared_mutex_dns.cpp)

find_package(Threads REQUIRED)
target_link_libraries(01_thread_safe_stack Threads::Threads)
target_link_libraries(02_deadlock_avoidance Threads::Threads)
target_link_libraries(03_lock_flexibility Threads::Threads)
target_link_libraries(04_call_once_singleton Threads::Threads)
target_link_libraries(05_shared_mutex_dns Threads::Threads)



