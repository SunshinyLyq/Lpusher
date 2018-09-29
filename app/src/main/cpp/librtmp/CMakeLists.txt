
cmake_minimum_required(VERSION 3.4.1)



file(GLOB source_file *.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")

add_library(
             rtmp
             STATIC

              ${source_file})

