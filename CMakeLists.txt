cmake_minimum_required(VERSION 3.8)
project(scalerel2)

add_subdirectory(./mylib)
add_executable(main main.cpp)
target_link_libraries(main reliabilitylib)