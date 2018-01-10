cmake_minimum_required(VERSION 3.5)

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
find_package(Eigen3 3.3 QUIET)

option(BUILD_TESTING OFF)
option(BUILD_EXAMPLES OFF)
subdirs(${VCL_SOURCE_DIR}/src/externals/cctz)
subdirs(${VCL_SOURCE_DIR}/src/externals/abseil)

option(FMT_TEST OFF)
option(FMT_USE_CPP11 ON)
subdirs(${VCL_SOURCE_DIR}/src/externals/fmtlib)
subdirs(${VCL_SOURCE_DIR}/src/externals/gsl)

option(BuildTests OFF)
subdirs(${VCL_SOURCE_DIR}/src/externals/json)

find_package(EGL QUIET)
if(EGL_FOUND)
	add_library(egl UNKNOWN IMPORTED)
	set_property(TARGET egl PROPERTY TARGET_INCLUDE_DIRECTORIES "${EGL_INCLUDE_DIR}")
	set_property(TARGET egl PROPERTY IMPORTED_LOCATION "${EGL_LIBRARY}")
else()
	subdirs(${VCL_SOURCE_DIR}/src/externals/EGL/EGL)
endif()

project(vcl.demos)

subdirs(${VCL_SOURCE_DIR}/src/libs/vcl.core)
subdirs(${VCL_SOURCE_DIR}/src/libs/vcl.graphics)

subdirs(graphics/colourtemperature)
subdirs(graphics/wrinkledsurfaces)
