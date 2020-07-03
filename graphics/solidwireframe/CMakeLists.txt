project(solidwireframe)

# Status message
message(STATUS "Configuring 'solidwireframe'")

# GLSL compiler
include(${VCL_SOURCE_DIR}/src/cmake/VCLCompileGLSL.cmake)

# Find includes in corresponding build directories
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

set(INC
	../application.h
	../basescene.h
)

set(SRC
	main.cpp
)

set(SHADERS
	shaders/solidwireframe.h
	shaders/solidwireframe.vert
	shaders/solidwireframe.geom
	shaders/solidwireframe.frag

	shaders/solidwireframe.glsl
)

source_group("shaders" FILES ${SHADERS})
source_group("" FILES ${SRC} ${INC})

# Access the include directories in order to compile the GLSL shaders
get_property(CURR_INC_DIRS_0 TARGET vcl_graphics PROPERTY INCLUDE_DIRECTORIES)
list(APPEND CURR_INC_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CURR_INC_DIRS_0})

vclcompileglsl(
	${PROJECT_SOURCE_DIR}/shaders/solidwireframe.vert
	"opengl"
	"SolidWireframeVert"
	"${CURR_INC_DIRS}"
	COMPILEDSHADERS_0
)
vclcompileglsl(
	${PROJECT_SOURCE_DIR}/shaders/solidwireframe.geom
	"opengl"
	"SolidWireframeGeom"
	"${CURR_INC_DIRS}"
	COMPILEDSHADERS_1
)
vclcompileglsl(
	${PROJECT_SOURCE_DIR}/shaders/solidwireframe.frag
	"opengl"
	"SolidWireframeFrag"
	"${CURR_INC_DIRS}"
	COMPILEDSHADERS_2
)
set(COMPILEDSHADERS ${COMPILEDSHADERS_0} ${COMPILEDSHADERS_1} ${COMPILEDSHADERS_2})

# Include dependencies
include_directories(${VCL_NANOGUI_INCLUDE})

add_executable(solidwireframe ${SRC} ${INC} ${SHADERS} ${COMPILEDSHADERS})
set_target_properties(solidwireframe PROPERTIES FOLDER graphics)

target_link_libraries(solidwireframe
	vcl.geometry
	vcl_graphics
	glfw
	imgui
)
