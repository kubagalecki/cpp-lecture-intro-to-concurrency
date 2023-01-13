cmake_minimum_required(VERSION 3.5)
project(Task_Queue_Lecture)

find_package(benchmark REQUIRED)
find_package(TBB REQUIRED)

add_executable(Task_Queue_Lecture main.cpp)
set_target_properties(Task_Queue_Lecture PROPERTIES CXX_STANDARD 23)
target_link_libraries(Task_Queue_Lecture TBB::tbb benchmark::benchmark)