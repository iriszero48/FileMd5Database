cmake_minimum_required(VERSION 3.9)
project(fmd)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenMP REQUIRED)
find_package(Boost 1.66.0 COMPONENTS system locale filesystem serialization)

if (Boost_FOUND AND OpenMP_CXX_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})

    add_executable(fmd main.cpp Log.h Path.h TupleSerialization.h CSV.h CSV.cpp FileMd5Database.h FileMd5Database.cpp Md5.h Md5.cpp String.h String.cpp FileMd5DatabaseSerialization.h FileMd5DatabaseSerialization.cpp File.h File.cpp)

    target_link_libraries(fmd ${Boost_LIBRARIES} OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "Required Boost/OpenMP packages not found.")
endif()
