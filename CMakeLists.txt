cmake_minimum_required(VERSION 2.8)

project(mintraprox)

add_subdirectory(src)
enable_testing()
include (CTest)

add_subdirectory(test)
