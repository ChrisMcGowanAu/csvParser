cmake_minimum_required(VERSION 3.21)
project(csvParser C CXX)
set(
    C_SOURCE_FILES
    csvParser.c
    csvTest.c
)
set(
    CPP_SOURCE_FILES
    csvParser.cpp
    csvTest.cpp
)

set(CMAKE_C_STANDARD 17)

add_executable(cParserTest ${C_SOURCE_FILES} )

set(CMAKE_CXX_STANDARD 17)
add_executable(cppParserTest ${CPP_SOURCE_FILES} )
