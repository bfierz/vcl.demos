/*
 * This file is part of the Visual Computing Library (VCL) release under the
 * MIT license.
 *
 * Copyright (c) 2018 Basil Fierz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// VCL configuration
#include <vcl/config/global.h>
#include <vcl/config/opengl.h>

// C++ standard library
#include <iostream>

// VCL
#include <vcl/geometry/meshfactory.h>
#include <vcl/graphics/opengl/glsl/uniformbuffer.h>
#include <vcl/graphics/opengl/context.h>
#include <vcl/graphics/runtime/opengl/resource/shader.h>
#include <vcl/graphics/runtime/opengl/resource/texture2d.h>
#include <vcl/graphics/runtime/opengl/state/pipelinestate.h>
#include <vcl/graphics/runtime/opengl/graphicsengine.h>
#include <vcl/graphics/camera.h>
#include <vcl/graphics/trackballcameracontroller.h>

#include "../application.h"
#include "../basescene.h"

#include "shaders/solidwireframe.h"
#include "solidwireframe.vert.spv.h"
#include "solidwireframe.geom.spv.h"
#include "solidwireframe.frag.spv.h"

// Force the use of the NVIDIA GPU in an Optimus system
extern "C"
{
	_declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}

class SolidWireframeExample : public BaseScene
{
	VCL_DECLARE_METAOBJECT(SolidWireframeExample)
public:
	SolidWireframeExample()
	{
		using Vcl::Graphics::Runtime::OpenGL::Buffer;
		using Vcl::Graphics::Runtime::OpenGL::PipelineState;
		using Vcl::Graphics::Runtime::OpenGL::RasterizerState;
		using Vcl::Graphics::Runtime::OpenGL::Shader;
		using Vcl::Graphics::Runtime::OpenGL::ShaderProgramDescription;
		using Vcl::Graphics::Runtime::OpenGL::ShaderProgram;
		using Vcl::Graphics::Runtime::BufferDescription;
		using Vcl::Graphics::Runtime::BufferInitData;
		using Vcl::Graphics::Runtime::BufferUsage;
		using Vcl::Graphics::Runtime::FillModeMethod;
		using Vcl::Graphics::Runtime::InputLayoutDescription;
		using Vcl::Graphics::Runtime::PipelineStateDescription;
		using Vcl::Graphics::Runtime::RasterizerDescription;
		using Vcl::Graphics::Runtime::ShaderType;
		using Vcl::Graphics::Runtime::VertexDataClassification;
		using Vcl::Graphics::Camera;
		using Vcl::Graphics::SurfaceFormat;

		// Initialize the graphics engine
		Vcl::Graphics::OpenGL::Context::initExtensions();
		Vcl::Graphics::OpenGL::Context::setupDebugMessaging();
		_engine = std::make_unique<Vcl::Graphics::Runtime::OpenGL::GraphicsEngine>();

		// Check availability of features
		if (!Shader::isSpirvSupported())
			throw std::runtime_error("SPIR-V is not supported.");

		// Initialize content
		_camera = std::make_unique<Camera>(std::make_shared<Vcl::Graphics::OpenGL::MatrixFactory>());
		_camera->encloseInFrustum({ 0, 0, 0 }, { 0, -1, 1 }, 2.0f, { 0, 0, 1 });

		_cameraController = std::make_unique<Vcl::Graphics::TrackballCameraController>();
		_cameraController->setCamera(_camera.get());

		// Initialize solid-wireframe shader
		InputLayoutDescription layout =
		{
			{
				{ 0, 12, VertexDataClassification::VertexDataPerObject },
				{ 1, 12, VertexDataClassification::VertexDataPerObject }},
			{
				{ "position", SurfaceFormat::R32G32B32_FLOAT, 1, 0,  0 },
				{ "normal",   SurfaceFormat::R32G32B32_FLOAT, 1, 0, 12 },
			}
		};

		Shader solid_wireframe_vert{ ShaderType::VertexShader,   0, SolidWireframeVert };
		Shader solid_wireframe_geom{ ShaderType::GeometryShader, 0, SolidWireframeGeom };
		Shader solid_wireframe_frag{ ShaderType::FragmentShader, 0, SolidWireframeFrag };
		PipelineStateDescription solid_wireframe_ps_desc;
		solid_wireframe_ps_desc.InputLayout = layout;
		solid_wireframe_ps_desc.VertexShader = &solid_wireframe_vert;
		solid_wireframe_ps_desc.GeometryShader = &solid_wireframe_geom;
		solid_wireframe_ps_desc.FragmentShader = &solid_wireframe_frag;
		_solidwireframePS = std::make_unique<PipelineState>(solid_wireframe_ps_desc);

		// Initialize the geometry
		const auto mesh = Vcl::Geometry::TriMeshFactory::createTorus(1.0f, 0.4f, 20, 20);

		struct Vertex
		{
			Vertex(const Eigen::Vector3f& p, const Eigen::Vector3f& n)
				: P(p), N(n)
			{}

			Eigen::Vector3f P;
			Eigen::Vector3f N;
		};
		std::vector<Vertex> vb;
		vb.reserve(mesh->nrFaces() * 3);

		auto faces = mesh->faces();
		auto positions = mesh->vertices();
		auto normals = mesh->vertexProperty<Eigen::Vector3f>("Normals");
		for (uint32_t f = 0; f < mesh->nrFaces(); ++f)
		{
			const auto face = faces[Vcl::Geometry::TriMesh::FaceId{ f }];
			vb.emplace_back(positions[face[0]], normals[face[0]]);
			vb.emplace_back(positions[face[1]], normals[face[1]]);
			vb.emplace_back(positions[face[2]], normals[face[2]]);
		}

		Vcl::Graphics::Runtime::BufferDescription buffer_desc;
		buffer_desc.Usage = BufferUsage::Vertex;
		buffer_desc.SizeInBytes = static_cast<uint32_t>(vb.size() * sizeof(Vertex));

		BufferInitData buffer_data;
		buffer_data.Data = vb.data();
		buffer_data.SizeInBytes = static_cast<uint32_t>(vb.size() * sizeof(Vertex));

		_meshGeometry = std::make_unique<Buffer>(buffer_desc, &buffer_data);
		_nrMeshVertices = vb.size();
	}

	Colour3f colour() const { return _colour; }
	void setColour(Colour3f val) { _colour = val; }

	float thickness() const { return _thickness; }
	void setThickness(float val) { _thickness = val; }

	float smoothing() const { return _smoothing; }
	void setSmoothing(float val) { _smoothing = val; }

public:
	void onMouseButton(Application& app, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			double x, y;
			glfwGetCursorPos(app.window(), &x, &y);
			_cameraController->startRotate((float)x / (float)app.width(), (float)y / (float)app.height());
		}
		else
		{
			_cameraController->endRotate();
		}
	}

	void onMouseMove(Application& app, double xpos, double ypos)
	{
		_cameraController->rotate((float)xpos / (float)app.width(), (float)ypos / (float)app.height());
	}

public:
	void draw(Application& app) override
	{
		_engine->beginFrame();

		_engine->clear(0, Eigen::Vector4f{0.0f, 0.0f, 0.0f, 1.0f});
		_engine->clear(1.0f);

		// View on the scene
		auto cbuf_camera = _engine->requestPerFrameConstantBuffer<PerFrameCameraData>();
		cbuf_camera->Viewport = vec4(0, 0, app.width(), app.height());
		cbuf_camera->Frustum;
		cbuf_camera->ViewMatrix = mat4(_camera->view());
		cbuf_camera->ProjectionMatrix = mat4(_camera->projection());
		_engine->setConstantBuffer(0, std::move(cbuf_camera));

		Eigen::Matrix4f M = _cameraController->currObjectTransformation();
		renderScene(Vcl::Graphics::Runtime::PrimitiveType::Trianglelist, _engine.get(), _solidwireframePS, M);
		
		_engine->endFrame();
	}

private:
	void renderScene
	(
		Vcl::Graphics::Runtime::PrimitiveType primitive_type,
		Vcl::Graphics::Runtime::GraphicsEngine* cmd_queue,
		Vcl::ref_ptr<Vcl::Graphics::Runtime::PipelineState> ps,
		const Eigen::Matrix4f& M
	)
	{
		// Configure the layout
		cmd_queue->setPipelineState(ps);

		// View on the scene
		auto cbuf_transform = cmd_queue->requestPerFrameConstantBuffer<ObjectTransformData>();
		cbuf_transform->ModelMatrix = M;
		cbuf_transform->NormalMatrix = mat4((_camera->view() * M).inverse().transpose());
		cmd_queue->setConstantBuffer(1, std::move(cbuf_transform));

		// View on the scene
		auto cbuf_config= cmd_queue->requestPerFrameConstantBuffer<SolidWireframeData>();
		cbuf_config->Colour.x = _colour.r;
		cbuf_config->Colour.y = _colour.g;
		cbuf_config->Colour.z = _colour.b;
		cbuf_config->Smoothing = _smoothing;
		cbuf_config->Thickness = _thickness;
		cmd_queue->setConstantBuffer(2, std::move(cbuf_config));

		// Render the quad
		cmd_queue->setVertexBuffer(0, *_meshGeometry, 0, 24);
		cmd_queue->setPrimitiveType(primitive_type, 3);
		cmd_queue->draw(_nrMeshVertices);
	}
	
private:
	std::unique_ptr<Vcl::Graphics::Runtime::GraphicsEngine> _engine;

private:
	std::unique_ptr<Vcl::Graphics::TrackballCameraController> _cameraController;

private:
	std::unique_ptr<Vcl::Graphics::Camera> _camera;

	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _solidwireframePS;

	//! Geometry (position, normal)
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Buffer> _meshGeometry;

	//! Number of vertices
	uint32_t _nrMeshVertices;

	//! Colour of the overlay
	Colour3f _colour{ 0, 0, 0 };

	//! Smoothness of the generated overlay
	float _smoothing{ 1.0f };

	//! Thickness of the lines
	float _thickness{ 1.0f };
};

VCL_RTTI_BASES(SolidWireframeExample, BaseScene)

VCL_RTTI_CTOR_TABLE_BEGIN(SolidWireframeExample)
	Vcl::RTTI::Constructor<SolidWireframeExample>()
VCL_RTTI_CTOR_TABLE_END(SolidWireframeExample)

VCL_RTTI_ATTR_TABLE_BEGIN(SolidWireframeExample)
	Vcl::RTTI::Attribute<SolidWireframeExample, Colour3f>{"Colour", &SolidWireframeExample::colour, &SolidWireframeExample::setColour},
	Vcl::RTTI::Attribute<SolidWireframeExample, float>{"Smoothing", &SolidWireframeExample::smoothing, &SolidWireframeExample::setSmoothing},
	Vcl::RTTI::Attribute<SolidWireframeExample, float>{"Thickness", &SolidWireframeExample::thickness, &SolidWireframeExample::setThickness}
VCL_RTTI_ATTR_TABLE_END(SolidWireframeExample)

VCL_DEFINE_METAOBJECT(SolidWireframeExample)
{
	VCL_RTTI_REGISTER_BASES(SolidWireframeExample);
	VCL_RTTI_REGISTER_CTORS(SolidWireframeExample);
	VCL_RTTI_REGISTER_ATTRS(SolidWireframeExample);
}

int main(int /* argc */, char ** /* argv */)
{
	Application app{ "VCL Solid Wireframe Example", 768, 768 };

	// Demo content
	SolidWireframeExample scene;
	app.setMouseButtonCallback([&scene](Application& app, int button, int action, int mods) {scene.onMouseButton(app, button, action, mods); });
	app.setMouseMoveCallback([&scene](Application& app, double xpos, double ypos) {scene.onMouseMove(app, xpos, ypos); });
	app.setSceneDrawCallback([&scene](Application& app) {scene.draw(app); });
	app.setUIDrawCallback([&scene](Application& app) {scene.drawUI(app); });

	return app.run();
}
