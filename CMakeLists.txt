# BubbleLEDClock2 — ESP-IDF top-level project file.
#
# Expects ESP-IDF >= 5.1.
# Build:
#   idf.py set-target esp32
#   idf.py menuconfig
#   idf.py build flash monitor

cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bubble_led_clock2)
