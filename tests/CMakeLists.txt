cmake_minimum_required(VERSION 3.22)

project(door_lock_host_tests C)

enable_testing()

add_executable(access_control_test
    access_control_test.c
    ../Core/Src/access_control.c
)

target_include_directories(access_control_test PRIVATE
    ../Core/Inc
)

add_test(NAME access_control_test COMMAND access_control_test)
