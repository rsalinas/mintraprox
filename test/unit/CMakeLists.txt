cmake_minimum_required (VERSION 2.8)

project(test)

include (CTest)
add_subdirectory(/usr/src/gtest gtest)


add_executable(main
	        main.cpp
		        /usr/src/gtest/src/gtest_main.cc)
target_link_libraries(main gtest )

add_test(NAME Main COMMAND main)
