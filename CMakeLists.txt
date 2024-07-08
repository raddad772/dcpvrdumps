cmake_minimum_required(VERSION 3.27)
project(dcpvrdumps C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(dcpvrdumps
        dcpvrdump.c
        dcpvrdump.h
        main.c)
