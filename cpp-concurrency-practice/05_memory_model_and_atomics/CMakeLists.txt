cmake_minimum_required(VERSION 3.10)
project(05MemoryModelAndAtomics)

set(CMAKE_CXX_STANDARD 20) # 支持 C++20 特性 (std::atmoic<std::shared_ptr> 等)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制使用指定的 C++ 标准

find_package(Threads REQUIRED)

macro(add_atomic_example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Threads::Threads)
endmacro()

add_atomic_example(01_lock_free_stack)
add_atomic_example(02_release_acquire)
add_atomic_example(03_atomic_flag_spinlock)
add_atomic_example(04_atomic_smart_ptr)
