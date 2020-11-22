# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(COMPONENTS "esptool_py esp_https_server json main")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(estacao_metereologica)