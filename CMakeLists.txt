#
# This file is part of the Visual Computing Library (VCL) release under the
# MIT license.
# 
# Copyright (c) 2018 Basil Fierz
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
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
if (VCL_DEMOS_STANDALONE_PROJECT)

	message(STATUS "Update VCL in '${VCL_SOURCE_DIR}'")
	set(VCL_SOURCE_DIR "${CMAKE_BINARY_DIR}/externals/vcl-src")

	if (NOT EXISTS "${VCL_SOURCE_DIR}")
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
			COMMAND git merge		
			COMMAND git submodule sync
			COMMAND git submodule update --init --recursive
			WORKING_DIRECTORY ${VCL_SOURCE_DIR}
		)
	endif()
	
	# Configure VCL
	option(VCL_BUILD_BENCHMARKS "" OFF)
	option(VCL_BUILD_EXAMPLES "" OFF)
	option(VCL_BUILD_TESTS "" OFF)
	option(VCL_BUILD_TOOLS "" ON)
	option(VCL_OPENGL_SUPPORT "" ON)
	add_subdirectory(${VCL_SOURCE_DIR}/src EXCLUDE_FROM_ALL)
endif()

# Add the windows library
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(externals/glfw)
set_target_properties(glfw PROPERTIES FOLDER 3rdParty)

# Configure ImGui
add_library(imgui STATIC "")
target_include_directories(imgui
	PUBLIC
		${GLEW_INCLUDE_DIR}
		externals/imgui
		graphics/imgui
)
target_sources(imgui
	PRIVATE
		graphics/imgui/imgui_impl_glfw.cpp
		graphics/imgui/imgui_impl_glfw.h
		graphics/imgui/imgui_impl_opengl3.cpp
		graphics/imgui/imgui_impl_opengl3.h
		externals/imgui/imconfig.h
		externals/imgui/imgui.cpp
		externals/imgui/imgui.h
		externals/imgui/imgui_draw.cpp
		externals/imgui/imgui_internal.h
		externals/imgui/stb_rect_pack.h
		externals/imgui/stb_textedit.h
		externals/imgui/stb_truetype.h
)
target_link_libraries(imgui 
	${GLEW_LIBRARY_RELEASE}
	glfw
)
set_target_properties(imgui PROPERTIES FOLDER 3rdParty)

# Actual demos
add_subdirectory(graphics/colourtemperature)
add_subdirectory(graphics/wrinkledsurfaces)
add_subdirectory(graphics/solidwireframe)
