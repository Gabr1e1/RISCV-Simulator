# cmake_minimum_required(VERSION <specify CMake version here>)
project(RISCV_Simulator)

set(CMAKE_CXX_STANDARD 17)

add_executable(RISCV_Simulator main.cpp utility.hpp executor.h executor.cpp instruction.h instruction.cpp)