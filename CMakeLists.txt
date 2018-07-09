cmake_minimum_required(VERSION 3.5)

project(vcl.demos)

# Determine whether this is a standalone project or included by other projects
set(VCL_DEMOS_STANDALONE_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(VCL_DEMOS_STANDALONE_PROJECT ON)

	# Configure all projects
	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
	SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
endif ()

# Load the VCL libraries from externally
set(VCL_SOURCE_DIR "${CMAKE_BINARY_DIR}/externals/vcl-src")
set(VCL_BINARY_DIR "${CMAKE_BINARY_DIR}/externals/vcl-build")

if (VCL_DEMOS_STANDALONE_PROJECT)
	message(STATUS "Update VCL in '${VCL_SOURCE_DIR}'")
	if(NOT EXISTS "${VCL_SOURCE_DIR}")
		file(MAKE_DIRECTORY ${VCL_SOURCE_DIR})
		execute_process(
			COMMAND git clone https://github.com/bfierz/vcl.git .
			COMMAND git checkout "master"
			COMMAND git submodule update --init --recursive
			WORKING_DIRECTORY ${VCL_SOURCE_DIR}
		)
	else()
		execute_process(
			COMMAND git fetch --all --prune
			COMMAND git checkout "master"
			COMMAND git merge		
			COMMAND git submodule sync
			COMMAND git submodule update --init --recursive
			WORKING_DIRECTORY ${VCL_SOURCE_DIR}
		)
	endif()
endif()

# VCL base libraries
if (VCL_DEMOS_STANDALONE_PROJECT)
	option(VCL_BUILD_BENCHMARKS "" OFF)
	option(VCL_BUILD_EXAMPLES "" OFF)
	option(VCL_BUILD_TOOLS "" ON)
	option(VCL_BUILD_TESTS "" OFF)
	option(VCL_OPENGL_SUPPORT "" ON)
	add_subdirectory(${VCL_SOURCE_DIR}/src EXCLUDE_FROM_ALL)
endif()

# Actual demos
add_subdirectory(graphics/colourtemperature)
add_subdirectory(graphics/wrinkledsurfaces)
add_subdirectory(graphics/solidwireframe)
