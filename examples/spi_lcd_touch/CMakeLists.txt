cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS ../../components)
#include($ENV{HDF_PATH}/component.cmake)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(spi_lcd_touch)
