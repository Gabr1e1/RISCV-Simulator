# cmake_minimum_required(VERSION <specify CMake version here>)
project(RISCV_Simulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(code main.cpp utility.hpp executor.h executor.cpp instruction.h instruction.cpp)