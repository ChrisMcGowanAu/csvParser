cmake_minimum_required(VERSION 3.21)
project(csvParser C)
set(
    SOURCE_FILES_TEST
    csvParser.c
    csvTest.c
)

set(
    SOURCE_FILES
    csvParser.c
)

set(CMAKE_C_STANDARD 17)

add_library(csvParser STATIC ${SOURCE_FILES})

add_executable(csvParserTest ${SOURCE_FILES_TEST} )
