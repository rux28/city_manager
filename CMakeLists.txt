cmake_minimum_required(VERSION 3.28.3)
project(city_manager C)

set(CMAKE_C_STANDARD 11)

add_executable(city_manager
        main.c
        utils.c
        commands.c)
