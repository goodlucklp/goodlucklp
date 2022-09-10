cmake_minimum_required(VERSION 3.4.1)
project(inputdemo)
add_definitions("-Wall -g -pthread -lpthread")
add_executable(inputdemo main.cpp input_device_manager.cpp)
