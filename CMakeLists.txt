cmake_minimum_required(VERSION 4.1)
project(C_osc C)

set(CMAKE_C_STANDARD 23)

add_executable(C_osc sendOSC.c
        )
