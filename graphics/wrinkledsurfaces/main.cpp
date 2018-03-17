/*
 * This file is part of the Visual Computing Library (VCL) release under the
 * MIT license.
 *
 * Copyright (c) 2016 Basil Fierz
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

// GSL
#include <gsl/gsl>

// NanoGUI
#include <nanogui/combobox.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/window.h>

// VCL
#include <vcl/graphics/opengl/glsl/uniformbuffer.h>
#include <vcl/graphics/opengl/context.h>
#include <vcl/graphics/runtime/opengl/resource/shader.h>
#include <vcl/graphics/runtime/opengl/resource/texture2d.h>
#include <vcl/graphics/runtime/opengl/state/pipelinestate.h>
#include <vcl/graphics/runtime/opengl/graphicsengine.h>
#include <vcl/graphics/camera.h>
#include <vcl/graphics/trackballcameracontroller.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shaders/wrinkledsurfaces.h"
#include "wrinkledsurfaces.vert.spv.h"
#include "wrinkledsurfaces.cont.spv.h"
#include "wrinkledsurfaces.eval.spv.h"
#include "wrinkledsurfaces.frag.spv.h"

// Force the use of the NVIDIA GPU in an Optimus system
extern "C"
{
	_declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}

using ImageType = std::unique_ptr<uint8_t[], void(*)(void*)>;

class WrinkledSurfacesExample : public nanogui::Screen
{
public:
	WrinkledSurfacesExample()
	: nanogui::Screen(Eigen::Vector2i(768, 768), "VCL Wrinkled Surfaces Example")
	{
		using namespace nanogui;

		using Vcl::Graphics::Runtime::OpenGL::PipelineState;
		using Vcl::Graphics::Runtime::OpenGL::RasterizerState;
		using Vcl::Graphics::Runtime::OpenGL::Shader;
		using Vcl::Graphics::Runtime::OpenGL::ShaderProgramDescription;
		using Vcl::Graphics::Runtime::OpenGL::ShaderProgram;
		using Vcl::Graphics::Runtime::FillMode;
		using Vcl::Graphics::Runtime::PipelineStateDescription;
		using Vcl::Graphics::Runtime::RasterizerDescription;
		using Vcl::Graphics::Runtime::ShaderType;
		using Vcl::Graphics::Camera;
		using Vcl::Graphics::SurfaceFormat;

		// Initialize the graphics engine
		Vcl::Graphics::OpenGL::Context::initExtensions();
		Vcl::Graphics::OpenGL::Context::setupDebugMessaging();
		_engine = std::make_unique<Vcl::Graphics::Runtime::OpenGL::GraphicsEngine>();

		// Check availability of features
		if (!Shader::isSpirvSupported())
			throw std::runtime_error("SPIR-V is not supported.");

		Window *window = new Window(this, "Detail Method");
		window->setPosition(Vector2i(15, 15));
		window->setLayout(new GroupLayout());

		Widget *panel = new Widget(window);
		panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));

		new Label(panel, "Method", "sans-bold");
		
		auto* method_selection = new ComboBox(panel, { "None", "Object Space", "Tangent Space", "Mikkelsen", "Displacements"});
		method_selection->setCallback([this](int idx)
		{
			_bumpMethod = idx;
		});
		
		performLayout();

		// Initialize content
		_camera = std::make_unique<Camera>(std::make_shared<Vcl::Graphics::OpenGL::MatrixFactory>());
		_camera->encloseInFrustum({ 0, 0, 0 }, { 0, -1, 1 }, 2.0f, { 0, 0, 1 });

		_cameraController = std::make_unique<Vcl::Graphics::TrackballCameraController>();
		_cameraController->setCamera(_camera.get());

		// Rasterization configuration
		RasterizerDescription raster_desc;
		//raster_desc.FillMode = FillMode::Wireframe;

		// Shader specializations
		std::array<unsigned int, 1> indices = { 0 };
		std::array<unsigned int, 1> simple_shader = { 0 };
		std::array<unsigned int, 1> objectspace_shader = { 1 };
		std::array<unsigned int, 1> tangentspace_shader = { 2 };
		std::array<unsigned int, 1> perturbnormal_shader = { 3 };
		std::array<unsigned int, 1> displacement_shader = { 4 };

		// Initialize simple shader
		Shader simple_vert{ ShaderType::VertexShader,   0, WrinkledSurfacesVert };
		Shader simple_frag{ ShaderType::FragmentShader, 0, WrinkledSurfacesFrag, indices, simple_shader };
		PipelineStateDescription simple_ps_desc;
		simple_ps_desc.VertexShader = &simple_vert;
		simple_ps_desc.FragmentShader = &simple_frag;
		_simplePS = std::make_unique<PipelineState>(simple_ps_desc);
		_simplePS->program().setUniform("DetailModeUniform", 0u);
		
		// Initialize tangent-space normal mapping shader
		Shader tangentspace_vert{ ShaderType::VertexShader,   0, WrinkledSurfacesVert };
		Shader tangentspace_frag{ ShaderType::FragmentShader, 0, WrinkledSurfacesFrag, indices, tangentspace_shader };
		PipelineStateDescription tangentspace_ps_desc;
		tangentspace_ps_desc.VertexShader = &tangentspace_vert;
		tangentspace_ps_desc.FragmentShader = &tangentspace_frag;
		_tangentNormalmapPS = std::make_unique<PipelineState>(tangentspace_ps_desc);
		_tangentNormalmapPS->program().setUniform("DetailModeUniform", 2u);

		// Initialize Mikkelsen bump mapping shader
		Shader perturb_vert{ ShaderType::VertexShader,   0, WrinkledSurfacesVert };
		Shader perturb_frag{ ShaderType::FragmentShader, 0, WrinkledSurfacesFrag, indices, perturbnormal_shader };
		PipelineStateDescription perturb_ps_desc;
		perturb_ps_desc.VertexShader = &perturb_vert;
		perturb_ps_desc.FragmentShader = &perturb_frag;
		_perturbNormalPS = std::make_unique<PipelineState>(perturb_ps_desc);
		_perturbNormalPS->program().setUniform("DetailModeUniform", 3u);
		
		// Initialize displacement mapping shader
		Shader disp_vert{ ShaderType::VertexShader,     0, WrinkledSurfacesVert };
		Shader disp_cont{ ShaderType::ControlShader,    0, WrinkledSurfacesCont };
		Shader disp_eval{ ShaderType::EvaluationShader, 0, WrinkledSurfacesEval };
		Shader disp_frag{ ShaderType::FragmentShader,   0, WrinkledSurfacesFrag, indices, displacement_shader };
		PipelineStateDescription disp_ps_desc;
		disp_ps_desc.Rasterizer = raster_desc;
		disp_ps_desc.VertexShader = &disp_vert;
		disp_ps_desc.TessControlShader = &disp_cont;
		disp_ps_desc.TessEvalShader = &disp_eval;
		disp_ps_desc.FragmentShader = &disp_frag;
		_displacementPS = std::make_unique<PipelineState>(disp_ps_desc);
		_displacementPS->program().setUniform("DetailModeUniform", 4u);

		// Load texture resources
		_diffuseMap = loadTexture("textures/diffuse.png");
		_specularMap = loadTexture("textures/specular.png");
		_normalMap = loadTexture("textures/normal.png");
		_heightMap = loadTexture("textures/height.png");
	}

public:
	bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override
	{
		if (Widget::mouseButtonEvent(p, button, down, modifiers))
			return true;

		if (down)
		{
			_cameraController->startRotate((float) p.x() / (float) width(), (float) p.y() / (float) height());
		}
		else
		{
			_cameraController->endRotate();
		}

		return true;
	}

	bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override
	{
		if (Widget::mouseMotionEvent(p, rel, button, modifiers))
			return true;

		_cameraController->rotate((float)p.x() / (float)width(), (float)p.y() / (float)height());
		return true;
	}

public:
	void drawContents() override
	{
		_engine->beginFrame();

		_engine->clear(0, Eigen::Vector4f{0.0f, 0.0f, 0.0f, 1.0f});
		_engine->clear(1.0f);

		// View on the scene
		auto cbuf_camera = _engine->requestPerFrameConstantBuffer<PerFrameCameraData>();
		cbuf_camera->Viewport = vec4(0, 0, width(), height());
		cbuf_camera->Frustum;
		cbuf_camera->ViewMatrix = mat4(_camera->view());
		cbuf_camera->ProjectionMatrix = mat4(_camera->projection());
		_engine->setConstantBuffer(0, cbuf_camera);

		Eigen::Matrix4f M = _cameraController->currObjectTransformation();
		switch (_bumpMethod)
		{
		case 0:
			renderScene(Vcl::Graphics::Runtime::PrimitiveType::Trianglelist, _engine.get(), _simplePS, M);
			break;
		case 1:
			break;
		case 2:
			renderScene(Vcl::Graphics::Runtime::PrimitiveType::Trianglelist, _engine.get(), _tangentNormalmapPS, M);
			break;
		case 3:
			renderScene(Vcl::Graphics::Runtime::PrimitiveType::Trianglelist, _engine.get(), _perturbNormalPS, M);
			break;
		case 4:
		{
			auto cbuf_tess = _engine->requestPerFrameConstantBuffer<TessellationData>();
			cbuf_tess->Level = 64;
			cbuf_tess->Midlevel = 0;
			cbuf_tess->HeightScale = 0.05f;
			_engine->setConstantBuffer(2, cbuf_tess);

			renderScene(Vcl::Graphics::Runtime::PrimitiveType::Patch, _engine.get(), _displacementPS, M);
			break;
		}
		}
		
		_engine->endFrame();
	}

private:
	void renderScene
	(
		Vcl::Graphics::Runtime::PrimitiveType primitive_type,
		gsl::not_null<Vcl::Graphics::Runtime::GraphicsEngine*> cmd_queue,
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
		cmd_queue->setConstantBuffer(1, cbuf_transform);
		
		// Textures
		cmd_queue->setTexture(0, *_diffuseMap);
		cmd_queue->setTexture(1, *_specularMap);
		cmd_queue->setTexture(2, *_heightMap);
		cmd_queue->setTexture(3, *_normalMap);

		// Render the quad
		cmd_queue->setPrimitiveType(primitive_type, 3);
		cmd_queue->draw(6);
	}

	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D> loadTexture(const char* filename)
	{
		using Vcl::Graphics::Runtime::OpenGL::Texture2D;
		using Vcl::Graphics::Runtime::Texture2DDescription;
		using Vcl::Graphics::Runtime::TextureResource;
		using Vcl::Graphics::SurfaceFormat;

		int force_channels = 0;
		int w, h, n;
		ImageType diffuse_data(stbi_load(filename, &w, &h, &n, force_channels), stbi_image_free);

		Texture2DDescription diffuse_tex_desc;
		diffuse_tex_desc.Width = w;
		diffuse_tex_desc.Height = h;
		diffuse_tex_desc.MipLevels = 1;
		diffuse_tex_desc.ArraySize = 1;
		diffuse_tex_desc.Format = SurfaceFormat::R8G8B8A8_UNORM;

		TextureResource diffuse_res;
		diffuse_res.Width = w;
		diffuse_res.Height = h;
		diffuse_res.Format = SurfaceFormat::R8G8B8A8_UNORM;
		diffuse_res.Data = diffuse_data.get();

		return std::make_unique<Texture2D>(diffuse_tex_desc, &diffuse_res);
	}

private:
	std::unique_ptr<Vcl::Graphics::Runtime::GraphicsEngine> _engine;

private:
	std::unique_ptr<Vcl::Graphics::TrackballCameraController> _cameraController;

private:
	std::unique_ptr<Vcl::Graphics::Camera> _camera;

	//! Selected bump-mapping technique
	int _bumpMethod{ 0 };
	
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D> _diffuseMap;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D> _specularMap;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D> _normalMap;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D> _heightMap;

	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _simplePS;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _objectNormalmapPS;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _tangentNormalmapPS;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _perturbNormalPS;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _displacementPS;
};

int main(int /* argc */, char ** /* argv */)
{
	try
	{
		nanogui::init();

		{
			nanogui::ref<WrinkledSurfacesExample> app = new WrinkledSurfacesExample();
			app->drawAll();
			app->setVisible(true);
			nanogui::mainloop();
		}

		nanogui::shutdown();
	}
	catch (const std::runtime_error &e)
	{
		std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
		std::cerr << error_msg << std::endl;
		return -1;
	}

	return 0;
}
