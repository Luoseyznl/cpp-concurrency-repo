cmake_minimum_required(VERSION 3.10)
project(07LockFreeConcurrentDataStructures)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

macro(add_ds_example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Threads::Threads)
endmacro()

add_ds_example(01_lock_free_stack)
add_ds_example(02_lock_free_queue)
add_ds_example(03_spsc_ring_buffer)
add_ds_example(04_hazard_pointers)
add_ds_example(05_epoch_based_reclamation)

