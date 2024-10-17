cmake_minimum_required(VERSION 3.5)
project(memory_game)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCES
	src/*.cpp
)

file(GLOB_RECURSE HEADERS
	src/*.h
)

include_directories(src)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
