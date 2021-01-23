cmake_minimum_required(VERSION 3.9)
project(fmd)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(UNIX)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
endif()

aux_source_directory(. srcs)
add_executable(fmd ${srcs})

if(WIN32)
	target_link_libraries(fmd)
else()
	target_link_libraries(fmd PUBLIC tbb Threads::Threads atomic)
endif()
