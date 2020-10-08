cmake_minimum_required(VERSION 2.8)
project(sgx_cmake)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/" ${CMAKE_MODULE_PATH})
list(APPEND CMAKE_MODULE_PATH ${DIDA_SOURCE_DIR}/cmake)


message("DIDA PATHS : ${DIDA_SOURCE_DIR}/cmake")
add_subdirectory(sample)
