cmake_minimum_required(VERSION 3.2)
project(mqtt LANGUAGES CXX)

add_library(mqtt static
    src/mqtt.cpp
    src/pack.cpp
)

target_compile_features(mqtt PUBLIC cxx_std_20)
