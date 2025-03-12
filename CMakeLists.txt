cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

# Std Module Config
# Set the experimental flag
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
	"0e5b6991-d74f-4b3d-a41c-cf096e0b2508" # current value as per https://www.kitware.com/import-std-in-cmake-3-30/
)

# tell CMake we want to use 'import std'
# will get enabled for all targets declared after this
set(CMAKE_CXX_MODULE_STD 1)

# Enable scan for dependencies
set(CXX_SCAN_FOR_MODULES ON)
# disable C++ extensions
set(CMAKE_CXX_EXTENSIONS OFF)
# force C++ version to 23
set(CMAKE_CXX_STANDARD 23)
# standard is required
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
include(${CMAKE_MODULE_PATH}/CPM.cmake)
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.8")
CPMAddPackage("gh:g-truc/glm#release-1-0-2")

# Project
project(sdl3-ter)

add_subdirectory(src)