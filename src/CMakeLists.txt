# Main Application

# include shader compiler
include(${CMAKE_MODULE_PATH}/compile-hlsl.cmake)

# name of this application/library
set(PRJ_APP_NAME "app")

# create executable for initial setup
add_executable(${PRJ_APP_NAME})

# ensure it's Standard C++ 23
target_compile_features(${PRJ_APP_NAME} PRIVATE cxx_std_23)

# set properties for GLM basically want this anytime it is used
LIST(APPEND glm_definitions
	GLM_FORCE_DEPTH_ZERO_TO_ONE # GLM clip space should be in Z-axis to 0 to 1
	GLM_FORCE_LEFT_HANDED       # GLM should use left-handed coordinates, +z goes into screen
	GLM_FORCE_RADIANS           # GLM should always use radians not degrees.
)

# Set Properties for Windows specific configurations
LIST(APPEND platform_definitions
	UNICODE _UNICODE            # Tell compiler we are using UNICODE
	_CRT_SECURE_NO_WARNINGS     # Don't want MSVC's CRT warnings
	NOMINMAX                    # Don't want Windows.h to define min and max macros
	WIN32_LEAN_AND_MEAN         # Windows.h should only include the basics
	WINDOWS                     # We are building for Windows
)

# set preprocessor defines
target_compile_definitions(${PRJ_APP_NAME}
	PRIVATE
		${platform_definitions}     # Plaform specific defintions, defined above
		${glm_definitions}          # GLM defintions, defined above
)

# precompiled headers
target_precompile_headers(${PRJ_APP_NAME}
	PRIVATE
		pch.hpp
)

# source files for this application
target_sources(${PRJ_APP_NAME}
	# Non C++ module source files
	PRIVATE
		main.cpp
	# magic encatations for C++ Modules
	PRIVATE FILE_SET app_modules TYPE CXX_MODULES FILES
		app.ixx
		clock.ixx
		io.ixx
		types.ixx
		pipeline.ixx
		camera.ixx
)

# libraries used by this application
target_link_libraries(${PRJ_APP_NAME}
	PRIVATE
	SDL3::SDL3 # SDL library version 3.2
	glm::glm   # Math Library
)

# shaders used by this application
target_hlsl_sources(${PRJ_APP_NAME}
	terrain.vs.hlsl : vs_6_4
	terrain.fs.hlsl : ps_6_4
)