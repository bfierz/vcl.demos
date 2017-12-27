cmake_minimum_required(VERSION 3.5)

project(vcl.demos)

# Load the VCL libraries from externally
set(VCL_SOURCE_DIR "${CMAKE_BINARY_DIR}/externals/vcl-src")
set(VCL_BINARY_DIR "${CMAKE_BINARY_DIR}/externals/vcl-build")

if(NOT EXISTS "${VCL_SOURCE_DIR}")
	file(MAKE_DIRECTORY ${VCL_SOURCE_DIR})
	execute_process(
		COMMAND git clone https://github.com/bfierz/vcl.git .
		COMMAND git pull
		COMMAND git checkout feature/opengl-base-work
		COMMAND git submodule update --init
		WORKING_DIRECTORY ${VCL_SOURCE_DIR}
	)
else()
	execute_process(
		COMMAND git pull
		COMMAND git submodule update --init
		WORKING_DIRECTORY ${VCL_SOURCE_DIR}
	)
endif()

# Configure the VCL compilation environment
set(CMAKE_MODULE_PATH "${VCL_SOURCE_DIR}/src/cmake")
include(${VCL_SOURCE_DIR}/src/cmake/VCLConfig.cmake)

# External libraries
set(EIGEN3_INCLUDE_DIR "${VCL_SOURCE_DIR}/src/externals/eigen")

subdirs(${VCL_SOURCE_DIR}/src/externals/EGL/EGL)
subdirs(${VCL_SOURCE_DIR}/src/externals/fmtlib)
subdirs(${VCL_SOURCE_DIR}/src/externals/gsl)
subdirs(${VCL_SOURCE_DIR}/src/externals/json)
set(BuildTests OFF CACHE BOOL "" FORCE)

subdirs(${VCL_SOURCE_DIR}/src/libs/vcl.core)
subdirs(${VCL_SOURCE_DIR}/src/libs/vcl.graphics)

subdirs(graphics/colourtemperature)
subdirs(graphics/wrinkledsurfaces)
