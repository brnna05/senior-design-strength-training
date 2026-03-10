cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(firmware)

set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(app PRIVATE 
    src/main.c
    src/ADC.c
    src/PPG.c
    src/IMU.c
)
