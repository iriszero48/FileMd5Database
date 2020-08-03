cmake_minimum_required(VERSION 3.9)
project(fmd)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(OpenMP REQUIRED)
find_package(Boost COMPONENTS system locale filesystem serialization)

if(Boost_FOUND AND OpenMP_CXX_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})

    aux_source_directory(. srcs)
    add_executable(fmd ${srcs})

    if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0 AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0)
        target_link_libraries(fmd ${Boost_LIBRARIES} OpenMP::OpenMP_CXX stdc++fs)
    else()
        target_link_libraries(fmd ${Boost_LIBRARIES} OpenMP::OpenMP_CXX)
    endif()
else()
    message(FATAL_ERROR "Required Boost/OpenMP packages not found.")
endif()
