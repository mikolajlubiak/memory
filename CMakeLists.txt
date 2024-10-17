cmake_minimum_required(VERSION 3.5)
project(memory_game)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Get FTXUI
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(ftxui
	GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
	GIT_TAG        v5.0.0
	GIT_PROGRESS   TRUE
	GIT_SHALLOW    TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ftxui)

# Sources
file(GLOB_RECURSE SOURCES
	src/*.cpp
)

file(GLOB_RECURSE HEADERS
	src/*.h
)

# Add binary
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME}
	PRIVATE ftxui::screen
	PRIVATE ftxui::dom
	PRIVATE ftxui::component  # Not needed for this example.
)
