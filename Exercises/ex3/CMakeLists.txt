cmake_minimum_required(VERSION 3.15)
project(ex3)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(ex3 MapReduceFramework.h MapReduceClient.h MapReduceFramework.cpp Sample_Client/SampleClient.cpp Barrier/Barrier.cpp)