cmake_minimum_required(VERSION 3.5)
project(mqtt LANGUAGES CXX)

add_library(mqtt STATIC
    mqtt.cpp
    pack.cpp
    network.cpp
)

target_compile_features(mqtt PUBLIC cxx_std_20)
