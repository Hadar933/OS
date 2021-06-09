cmake_minimum_required(VERSION 3.15)
project(ex3)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS -pthread)
set(CMAKE_BUILD_TYPE Debug)

add_executable(ex3 MapReduceFramework.h MapReduceClient.h MapReduceFramework.cpp Barrier/Barrier.cpp Atomic/atomicdemo.cpp Sample_Client/SampleClient.cpp)