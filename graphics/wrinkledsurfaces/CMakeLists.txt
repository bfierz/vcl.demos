project(wrinkledsurfaces)

# Status message
message(STATUS "Configuring 'wrinkledsurfaces'")

# GLSL compiler
include(${VCL_SOURCE_DIR}/src/cmake/VCLCompileGLSL.cmake)

# NanoGUI
set(VCL_NANOGUI_INCLUDE CACHE PATH "Include directory of NanoGUI")
set(VCL_NANOGUI_DEBUG_LIBRARY CACHE FILEPATH "NanoGUI library")
set(VCL_NANOGUI_RELEASE_LIBRARY CACHE FILEPATH "NanoGUI library")

# Find includes in corresponding build directories
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

set(INC
	stb_image.h
)

set(SRC
	main.cpp
)

set(SHADERS
	shaders/wrinkledsurfaces.h
	shaders/wrinkledsurfaces.vert
	shaders/wrinkledsurfaces.tesc
	shaders/wrinkledsurfaces.tese
	shaders/wrinkledsurfaces.frag

	shaders/perturbnormal.glsl
)

source_group("shaders" FILES ${SHADERS})
source_group("" FILES ${SRC} ${INC})

# Access the include directories in order to compile the GLSL shaders
get_property(CURR_INC_DIRS_0 TARGET vcl_graphics PROPERTY INCLUDE_DIRECTORIES)
list(APPEND CURR_INC_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CURR_INC_DIRS_0})

vclcompileglsl(
	${PROJECT_SOURCE_DIR}/shaders/wrinkledsurfaces.vert
	"WrinkledSurfacesVert"
	"${CURR_INC_DIRS}"
	COMPILEDSHADERS_0
)
vclcompileglsl(
	${PROJECT_SOURCE_DIR}/shaders/wrinkledsurfaces.tesc
	"WrinkledSurfacesCont"
	"${CURR_INC_DIRS}"
	COMPILEDSHADERS_1
)
vclcompileglsl(
	${PROJECT_SOURCE_DIR}/shaders/wrinkledsurfaces.tese
	"WrinkledSurfacesEval"
	"${CURR_INC_DIRS}"
	COMPILEDSHADERS_2
)
vclcompileglsl(
	${PROJECT_SOURCE_DIR}/shaders/wrinkledsurfaces.frag
	"WrinkledSurfacesFrag"
	"${CURR_INC_DIRS}"
	COMPILEDSHADERS_3
)
set(COMPILEDSHADERS ${COMPILEDSHADERS_0} ${COMPILEDSHADERS_1} ${COMPILEDSHADERS_2} ${COMPILEDSHADERS_3})

# Include dependencies
include_directories(${VCL_NANOGUI_INCLUDE})

add_executable(wrinkledsurfaces ${SRC} ${INC} ${SHADERS} ${COMPILEDSHADERS})
set_target_properties(wrinkledsurfaces PROPERTIES FOLDER graphics)

target_link_libraries(wrinkledsurfaces
	vcl_graphics
	debug ${VCL_NANOGUI_DEBUG_LIBRARY}
	optimized ${VCL_NANOGUI_RELEASE_LIBRARY}
)

# Copy the resources to the binary directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/textures" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
