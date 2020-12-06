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

// VCL
#include <vcl/core/enum.h>
#include <vcl/graphics/opengl/glsl/uniformbuffer.h>
#include <vcl/graphics/opengl/context.h>
#include <vcl/graphics/runtime/opengl/resource/shader.h>
#include <vcl/graphics/runtime/opengl/resource/texture2d.h>
#include <vcl/graphics/runtime/opengl/state/sampler.h>
#include <vcl/graphics/runtime/opengl/state/pipelinestate.h>
#include <vcl/graphics/runtime/opengl/graphicsengine.h>
#include <vcl/graphics/camera.h>
#include <vcl/graphics/trackballcameracontroller.h>

#include "../application.h"
#include "../basescene.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shaders/wrinkledsurfaces.h"
#include "wrinkledsurfaces.vert.spv.h"
#include "wrinkledsurfaces.tesc.spv.h"
#include "wrinkledsurfaces.tese.spv.h"
#include "wrinkledsurfaces.frag.spv.h"

// Force the use of the NVIDIA GPU in an Optimus system
extern "C"
{
	_declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}

using ImageType = std::unique_ptr<uint8_t[], void(*)(void*)>;

VCL_DECLARE_ENUM(Scene,
	Pyramid, 
	Wall,
	Dome
)

VCL_DECLARE_ENUM(DetailMethod,
	None,
	ObjectSpace,
	TangentSpace,
	Mikkelsen,
	Displacements
)

class WrinkledSurfacesExample : public BaseScene
{
	VCL_DECLARE_METAOBJECT(WrinkledSurfacesExample)
public:
	WrinkledSurfacesExample()
	{
		using Vcl::Graphics::Runtime::OpenGL::PipelineState;
		using Vcl::Graphics::Runtime::OpenGL::RasterizerState;
		using Vcl::Graphics::Runtime::OpenGL::Sampler;
		using Vcl::Graphics::Runtime::OpenGL::Shader;
		using Vcl::Graphics::Runtime::OpenGL::ShaderProgramDescription;
		using Vcl::Graphics::Runtime::OpenGL::ShaderProgram;
		using Vcl::Graphics::Runtime::FillModeMethod;
		using Vcl::Graphics::Runtime::FilterType;
		using Vcl::Graphics::Runtime::PipelineStateDescription;
		using Vcl::Graphics::Runtime::RasterizerDescription;
		using Vcl::Graphics::Runtime::SamplerDescription;
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
		Shader objectspace_vert{ ShaderType::VertexShader,   0, WrinkledSurfacesVert };
		Shader objectspace_frag{ ShaderType::FragmentShader, 0, WrinkledSurfacesFrag, indices, objectspace_shader };
		PipelineStateDescription objectspace_ps_desc;
		objectspace_ps_desc.VertexShader   = &objectspace_vert;
		objectspace_ps_desc.FragmentShader = &objectspace_frag;
		_objectNormalmapPS = std::make_unique<PipelineState>(objectspace_ps_desc);
		_objectNormalmapPS->program().setUniform("DetailModeUniform", 1u);

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

		// Create a linear sampler
		SamplerDescription desc;
		desc.Filter = FilterType::MinMagLinearMipPoint;
		_linearSampler = std::make_unique<Sampler>(desc);

		// Load texture resources
		auto textures = createPyramidTextures(2, 0.1f, 256, 8);
		_diffuseMap  [0] = std::move(textures[0]);
		_normalObjMap[0] = std::move(textures[1]);
		_normalTanMap[0] = std::move(textures[2]);
		_heightMap   [0] = std::move(textures[3]);

		_diffuseMap  [1] = loadTexture("textures/wall/diffuse.png");
		_normalObjMap[1] = loadTexture("textures/wall/normal_obj.png");
		_normalTanMap[1] = loadTexture("textures/wall/normal_tan.png");
		_heightMap   [1] = loadTexture("textures/wall/height.png");

		_diffuseMap  [2] = loadTexture("textures/dome/diffuse.png");
		_normalObjMap[2] = loadTexture("textures/dome/normal_obj.png");
		_normalTanMap[2] = loadTexture("textures/dome/normal_tan.png");
		_heightMap   [2] = loadTexture("textures/dome/height.png");
	}

	Scene scene() const { return _scene; }
	void setScene(Scene scene) { _scene = scene; }
	DetailMethod detailMethod() const { return _detailMethod; }
	void setDetailMethod(DetailMethod method) { _detailMethod = method; }

public:
	void onMouseButton(Application& app, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			double x, y;
			glfwGetCursorPos(app.window(), &x, &y);
			_cameraController->startRotate((float) x / (float) app.width(), (float) y / (float) app.height());
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
		cbuf_camera->Viewport = vec4(0, 0, (float)app.width(), (float)app.height());
		cbuf_camera->Frustum;
		cbuf_camera->ViewMatrix = mat4(_camera->view());
		cbuf_camera->ProjectionMatrix = mat4(_camera->projection());
		_engine->setConstantBuffer(0, std::move(cbuf_camera));

		Eigen::Matrix4f M = _cameraController->currObjectTransformation();
		switch (_detailMethod)
		{
		case DetailMethod::None:
			renderScene(Vcl::Graphics::Runtime::PrimitiveType::Trianglelist, _engine.get(), _simplePS, M);
			break;
		case DetailMethod::ObjectSpace:
			renderScene(Vcl::Graphics::Runtime::PrimitiveType::Trianglelist, _engine.get(), _objectNormalmapPS, M);
			break;
		case DetailMethod::TangentSpace:
			renderScene(Vcl::Graphics::Runtime::PrimitiveType::Trianglelist, _engine.get(), _tangentNormalmapPS, M);
			break;
		case DetailMethod::Mikkelsen:
			renderScene(Vcl::Graphics::Runtime::PrimitiveType::Trianglelist, _engine.get(), _perturbNormalPS, M);
			break;
		case DetailMethod::Displacements:
		{
			auto cbuf_tess = _engine->requestPerFrameConstantBuffer<TessellationData>();
			cbuf_tess->Level = 64;
			cbuf_tess->Midlevel = 127.0f/255.0f;
			cbuf_tess->HeightScale = 0.01f;
			_engine->setConstantBuffer(2, std::move(cbuf_tess));

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

		// Samplers
		cmd_queue->setSampler(0, *_linearSampler);
		cmd_queue->setSampler(1, *_linearSampler);
		cmd_queue->setSampler(2, *_linearSampler);
		cmd_queue->setSampler(3, *_linearSampler);

		// Textures
		cmd_queue->setTexture(0, *_diffuseMap[(int)_scene]);
		cmd_queue->setTexture(1, *_heightMap[(int)_scene]);
		cmd_queue->setTexture(2, *_normalObjMap[(int)_scene]);
		cmd_queue->setTexture(3, *_normalTanMap[(int)_scene]);

		// Render the quad
		cmd_queue->setPrimitiveType(primitive_type, 3);
		cmd_queue->draw(6);
	}

	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D> createTexture
	(
		uint32_t w, uint32_t h, Vcl::Graphics::SurfaceFormat tgt_fmt,
		const void* src, Vcl::Graphics::SurfaceFormat src_fmt
	) const
	{
		using Vcl::Graphics::Runtime::OpenGL::Texture2D;
		using Vcl::Graphics::Runtime::Texture2DDescription;
		using Vcl::Graphics::Runtime::TextureResource;

		Texture2DDescription diffuse_tex_desc;
		diffuse_tex_desc.Width = w;
		diffuse_tex_desc.Height = h;
		diffuse_tex_desc.MipLevels = 1;
		diffuse_tex_desc.ArraySize = 1;
		diffuse_tex_desc.Format = tgt_fmt;

		TextureResource diffuse_res;
		diffuse_res.Width = w;
		diffuse_res.Height = h;
		diffuse_res.Format = src_fmt;
		diffuse_res.Data = stdext::make_span(reinterpret_cast<const uint8_t*>(src), w*h*Vcl::Graphics::sizeInBytes(src_fmt));

		return std::make_unique<Texture2D>(diffuse_tex_desc, &diffuse_res);
	}

	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D> loadTexture(const char* filename) const
	{
		using Vcl::Graphics::SurfaceFormat;

		int force_channels = 0;
		int w, h, n;
		ImageType diffuse_data(stbi_load(filename, &w, &h, &n, force_channels), stbi_image_free);
		SurfaceFormat input_format = SurfaceFormat::R8G8B8A8_UNORM;
		if (n == 1)
			input_format = SurfaceFormat::R8_UNORM;
		if (n == 3)
			input_format = SurfaceFormat::R8G8B8_UNORM;

		return createTexture(w, h, SurfaceFormat::R8G8B8A8_UNORM, diffuse_data.get(), input_format);
	}

	std::array<std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D>, 4>
		createPyramidTextures
		(
			float size, float height, size_t resolution, size_t bumpmap_bpp
		) const
	{
		using Vcl::Graphics::Runtime::OpenGL::Texture2D;
		using Vcl::Graphics::SurfaceFormat;
		using RGBA8 = std::array<uint8_t, 4>;

		std::array<std::unique_ptr<Texture2D>, 4> textures;

		// Create a dummy albedo map in a medium grey
		RGBA8 blue = {   0,   0,   0, 255 };
		RGBA8 grey = { 127, 127, 127, 255 };
		std::vector<RGBA8> albedo_map(resolution*resolution, grey);
		textures[0] = createTexture(resolution, resolution, SurfaceFormat::R8G8B8A8_UNORM, albedo_map.data(), SurfaceFormat::R8G8B8A8_UNORM);
		
		const float incr = size / (resolution - 1);
		const float lower = 0.1f * size;
		const float mid = 0.5f * size;
		const float upper = 0.9f * size;
		std::vector<float> height_map(resolution*resolution, 0);
		std::vector<RGBA8> normal_map(resolution*resolution, blue);
		for (size_t y = 0; y < resolution; y++)
		{
			for (size_t x = 0; x < resolution; x++)
			{
				const size_t idx = y * resolution + x;
				const float curr_x = x * incr;
				const float curr_y = y * incr;

				float height_x = 0;
				Eigen::Vector3f normal_x = Eigen::Vector3f::Zero();
				if (curr_x < lower)
				{
					height_x = 0;
					normal_x = {0, 0, height};
				}
				else if (curr_x < mid)
				{
					height_x = height * abs(curr_x - lower) / (mid - lower);
					normal_x = { -height, 0, mid - lower };
				}
				else if (curr_x < upper)
				{
					height_x = height * abs(upper - curr_x) / (upper - mid);
					normal_x = { height, 0, upper - mid };
				}
				else
				{
					height_x = 0;
					normal_x = { 0, 0, height };
				}

				float height_y = 0;
				Eigen::Vector3f normal_y = Eigen::Vector3f::Zero();
				if (curr_y < lower)
				{
					height_y = 0;
					normal_y = { 0, 0, height };
				}
				else if (curr_y < mid)
				{
					height_y = height * abs(curr_y - lower) / (mid - lower);
					normal_y = { 0, -height, mid - lower };
				}
				else if (curr_y < upper)
				{
					height_y = height * abs(upper - curr_y) / (upper - mid);
					normal_y = { 0, height, upper - mid };
				}
				else
				{
					height_y = 0;
					normal_y = { 0, 0, height };
				}

				if (height_x < height_y)
				{
					height_map[idx] = height_x;
					normal_x.normalize();
					normal_map[idx][0] = static_cast<uint8_t>(255 * (0.5f * normal_x[0] + 0.5f));
					normal_map[idx][1] = static_cast<uint8_t>(255 * (0.5f * normal_x[1] + 0.5f));
					normal_map[idx][2] = static_cast<uint8_t>(255 * (0.5f * normal_x[2] + 0.5f));
				}
				else
				{
					height_map[idx] = height_y;
					normal_y.normalize();
					normal_map[idx][0] = static_cast<uint8_t>(255 * (0.5f * normal_y[0] + 0.5f));
					normal_map[idx][1] = static_cast<uint8_t>(255 * (0.5f * normal_y[1] + 0.5f));
					normal_map[idx][2] = static_cast<uint8_t>(255 * (0.5f * normal_y[2] + 0.5f));
				}
			}
		}
		textures[1] = createTexture(resolution, resolution, SurfaceFormat::R8G8B8A8_UNORM, normal_map.data(), SurfaceFormat::R8G8B8A8_UNORM);
		textures[2] = createTexture(resolution, resolution, SurfaceFormat::R8G8B8A8_UNORM, normal_map.data(), SurfaceFormat::R8G8B8A8_UNORM);

		if (bumpmap_bpp == 8)
		{
			std::vector<uint8_t> quantized_height_map(resolution*resolution, 0);
			std::transform(height_map.begin(), height_map.end(), quantized_height_map.begin(), [height](float h)
			{
				return std::numeric_limits<uint8_t>::max() * h / height;
			});			
			textures[3] = createTexture(resolution, resolution, SurfaceFormat::R8_UNORM, quantized_height_map.data(), SurfaceFormat::R8_UNORM);
		}
		else if (bumpmap_bpp == 16)
		{
			std::vector<uint16_t> quantized_height_map(resolution*resolution, 0);
			std::transform(height_map.begin(), height_map.end(), quantized_height_map.begin(), [height](float h)
			{
				return std::numeric_limits<uint16_t>::max() * h / height;
			});
			textures[3] = createTexture(resolution, resolution, SurfaceFormat::R16_UNORM, quantized_height_map.data(), SurfaceFormat::R16_UNORM);
		}
		else if (bumpmap_bpp == 32)
		{
			std::vector<float> quantized_height_map(resolution*resolution, 0);
			std::transform(height_map.begin(), height_map.end(), quantized_height_map.begin(), [height](float h)
			{
				return h / height;
			});
			textures[3] = createTexture(resolution, resolution, SurfaceFormat::R32_FLOAT, quantized_height_map.data(), SurfaceFormat::R32_FLOAT);
		}

		return textures;
	}

private:
	std::unique_ptr<Vcl::Graphics::Runtime::GraphicsEngine> _engine;

private:
	std::unique_ptr<Vcl::Graphics::TrackballCameraController> _cameraController;

private:
	std::unique_ptr<Vcl::Graphics::Camera> _camera;

	//! Selected scene
	Scene _scene{ Scene::Pyramid };

	//! Selected bump-mapping technique
	DetailMethod _detailMethod{ DetailMethod::None };

	std::array<std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D>, 3> _diffuseMap;
	std::array<std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D>, 3> _normalObjMap;
	std::array<std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D>, 3> _normalTanMap;
	std::array<std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Texture2D>, 3> _heightMap;

	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::Sampler> _linearSampler;

	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _simplePS;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _objectNormalmapPS;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _tangentNormalmapPS;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _perturbNormalPS;
	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _displacementPS;
};

VCL_RTTI_BASES(WrinkledSurfacesExample, BaseScene)

VCL_RTTI_CTOR_TABLE_BEGIN(WrinkledSurfacesExample)
	Vcl::RTTI::Constructor<WrinkledSurfacesExample>()
VCL_RTTI_CTOR_TABLE_END(WrinkledSurfacesExample)

VCL_RTTI_ATTR_TABLE_BEGIN(WrinkledSurfacesExample)
	Vcl::RTTI::Attribute<WrinkledSurfacesExample, Scene>{"Scene", &WrinkledSurfacesExample::scene, &WrinkledSurfacesExample::setScene},
	Vcl::RTTI::Attribute<WrinkledSurfacesExample, DetailMethod>{"DetailMethod", &WrinkledSurfacesExample::detailMethod, &WrinkledSurfacesExample::setDetailMethod}
VCL_RTTI_ATTR_TABLE_END(WrinkledSurfacesExample)

VCL_DEFINE_METAOBJECT(WrinkledSurfacesExample)
{
	VCL_RTTI_REGISTER_BASES(WrinkledSurfacesExample);
	VCL_RTTI_REGISTER_CTORS(WrinkledSurfacesExample);
	VCL_RTTI_REGISTER_ATTRS(WrinkledSurfacesExample);
}
int main(int /* argc */, char ** /* argv */)
{
	Application app{ "VCL Wrinkled Surfaces Example", 768, 768 };

	// Demo content
	WrinkledSurfacesExample scene;
	app.setMouseButtonCallback([&scene](Application& app, int button, int action, int mods) {scene.onMouseButton(app, button, action, mods);});
	app.setMouseMoveCallback([&scene](Application& app, double xpos, double ypos) {scene.onMouseMove(app, xpos, ypos);});
	app.setSceneDrawCallback([&scene](Application& app) {scene.draw(app); });
	app.setUIDrawCallback([&scene](Application& app) {scene.drawUI(app); });

	return app.run();
}
