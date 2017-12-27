project(colourtemperature)

# Status message
message(STATUS "Configuring 'colourtemperature'")

# GLSL compiler
include(${VCL_SOURCE_DIR}/src/cmake/VCLCompileGLSL.cmake)

# NanoGUI
set(VCL_NANOGUI_INCLUDE CACHE PATH "Include directory of NanoGUI")
set(VCL_NANOGUI_DEBUG_LIBRARY CACHE FILEPATH "NanoGUI library")
set(VCL_NANOGUI_RELEASE_LIBRARY CACHE FILEPATH "NanoGUI library")

# Find includes in corresponding build directories
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

set(INC
)

set(SRC
	main.cpp
)

set(SHADERS
	shaders/temperature.h
	shaders/temperature.vert
	shaders/temperature.frag
	
	shaders/colourtemperature.glsl
)

source_group("shaders" FILES ${SHADERS})
source_group("" FILES ${SRC} ${INC})

# Access the include directories in order to compile the GLSL shaders
get_property(CURR_INC_DIRS_0 TARGET vcl_graphics PROPERTY INCLUDE_DIRECTORIES)
list(APPEND CURR_INC_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CURR_INC_DIRS_0})

vclcompileglsl(
	${PROJECT_SOURCE_DIR}/shaders/temperature.vert
	"TemperatureVert"
	"${CURR_INC_DIRS}"
	COMPILEDSHADERS_0
)
vclcompileglsl(
	${PROJECT_SOURCE_DIR}/shaders/temperature.frag
	"TemperatureFrag"
	"${CURR_INC_DIRS}"
	COMPILEDSHADERS_1
)
set(COMPILEDSHADERS ${COMPILEDSHADERS_0} ${COMPILEDSHADERS_1})

# Include dependencies
include_directories(${VCL_NANOGUI_INCLUDE})

add_executable(colourtemperature ${SRC} ${INC} ${SHADERS} ${COMPILEDSHADERS})
set_target_properties(colourtemperature PROPERTIES FOLDER graphics)

target_link_libraries(colourtemperature
	vcl_graphics
	debug ${VCL_NANOGUI_DEBUG_LIBRARY}
	optimized ${VCL_NANOGUI_RELEASE_LIBRARY}
)
