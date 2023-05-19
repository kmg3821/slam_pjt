cmake_minimum_required(VERSION 3.10)

project(pcl_test)

add_executable(pcl pcl.cpp)

find_package (PCL 1.12 REQUIRED COMPONENTS common visualization io)

include_directories(${PCL_INCLUDE_DIRS})

link_libraries(${PCL_LIBRARY_DIRS})

target_link_libraries(pcl ${PCL_LIBRARIES} -lpcl_filters)