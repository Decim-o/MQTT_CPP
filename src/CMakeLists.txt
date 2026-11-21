cmake_minimum_required(VERSION 3.5)
project(mqtt LANGUAGES CXX)

add_library(mqtt STATIC
    mqtt.cpp
    pack.cpp
    network.cpp
)

target_compile_features(mqtt PUBLIC cxx_std_20)

include(CTest)

if(BUILD_TESTING)
    add_executable(test_network tests/test_network.cpp)
    target_include_directories(test_network PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(test_network PRIVATE mqtt)
    add_test(NAME network COMMAND test_network)
    set_tests_properties(network PROPERTIES TIMEOUT 10)
endif()
