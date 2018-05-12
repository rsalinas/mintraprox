cmake_minimum_required(VERSION 2.8)

project(multisocks5d)
add_executable(${PROJECT_NAME} "main.cpp" "socks5.h" resolve.cpp resolve.h Select.cpp Select.h
        Socks5Client.cpp  Socks5Client.h  Socks5Server.cpp  Socks5Server.h Redirector.h Redirector.cpp
        Config.h Config.cpp log.h log.cpp
        IpcServer.h IpcServer.cpp
  	netcommon.h	
	)
target_compile_options(${PROJECT_NAME} PUBLIC "-Wall")
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(${PROJECT_NAME} PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(${PROJECT_NAME} "${CMAKE_THREAD_LIBS_INIT}")
endif()



