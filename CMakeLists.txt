cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

# Std Module Config
# Set the experimental flag
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
#	"0e5b6991-d74f-4b3d-a41c-cf096e0b2508" # current value as per https://www.kitware.com/import-std-in-cmake-3-30/
	"a9e1cf81-9932-4810-974b-6eccaf14e457" # updated value for 4.0.x
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
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.10")
CPMAddPackage("gh:g-truc/glm#release-1-0-2")
CPMAddPackage("gh:septag/dds-ktx#v1.1")
if (dds-ktx_ADDED)
	add_library(dds-ktx INTERFACE)
	target_include_directories(dds-ktx INTERFACE ${dds-ktx_SOURCE_DIR})
endif()

# Project
project(sdl3-ter)

add_subdirectory(src)