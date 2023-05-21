cmake_minimum_required(VERSION 3.10)

project(pcl_test)

add_executable(pcl pcl.cpp)

find_package (PCL 1.13)

include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

target_link_libraries(pcl ${PCL_LIBRARIES})