cmake_minimum_required(VERSION 3.15)
project(ex3)

set(CMAKE_CXX_STANDARD 11)

add_executable(ex3 MapReduceFramework.h MapReduceClient.h MapReduceFramework.cpp Barrier/Barrier.cpp Barrier/barrierdemo.cpp)