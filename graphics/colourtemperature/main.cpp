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
#include <vcl/config/eigen.h>
#include <vcl/config/opengl.h>

// C++ standard library
#include <chrono>
#include <exception>
#include <functional>
#include <iostream>

// GSL
#include <gsl/gsl>

// VCL
#include <vcl/rtti/metatype.h>
#include <vcl/rtti/metatypeconstructor.inl>

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

#include "shaders/temperature.h"
#include "temperature.vert.spv.h"
#include "temperature.frag.spv.h"

// Force the use of the NVIDIA GPU in an Optimus system
extern "C"
{
	_declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}

using ImageType = std::unique_ptr<uint8_t[], void(*)(void*)>;

class WrinkledSurfacesExample : public BaseScene
{
	VCL_DECLARE_METAOBJECT(WrinkledSurfacesExample)
public:
	WrinkledSurfacesExample()
	{
		using Vcl::Graphics::Runtime::OpenGL::PipelineState;
		using Vcl::Graphics::Runtime::OpenGL::RasterizerState;
		using Vcl::Graphics::Runtime::OpenGL::Shader;
		using Vcl::Graphics::Runtime::OpenGL::ShaderProgramDescription;
		using Vcl::Graphics::Runtime::OpenGL::ShaderProgram;
		using Vcl::Graphics::Runtime::FillModeMethod;
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

		// Initialize content
		_camera = std::make_unique<Camera>(std::make_shared<Vcl::Graphics::OpenGL::MatrixFactory>());
		_camera->encloseInFrustum({ 0, 0, 0 }, { 0, -1, 1 }, 2.0f, { 0, 0, 1 });

		_cameraController = std::make_unique<Vcl::Graphics::TrackballCameraController>();
		_cameraController->setCamera(_camera.get());

		// Initialize simple shader
		Shader temperature_vert{ ShaderType::VertexShader,   0, TemperatureVert };
		Shader temperature_frag{ ShaderType::FragmentShader, 0, TemperatureFrag };
		PipelineStateDescription temperature_ps_desc;
		temperature_ps_desc.VertexShader = &temperature_vert;
		temperature_ps_desc.FragmentShader = &temperature_frag;
		_temperaturePS = std::make_unique<PipelineState>(temperature_ps_desc);
	}

public:
	void draw(Application& app) override
	{
		const std::chrono::nanoseconds time_increment = std::chrono::nanoseconds{ _animation_length } / 100;
		const std::chrono::nanoseconds now = std::chrono::steady_clock::now().time_since_epoch();
		if (_last_check + time_increment < now)
		{
			_animation_value += 0.01f;
			_last_check = now;
		}

		_engine->beginFrame();

		_engine->clear(0, Eigen::Vector4f{0.0f, 0.0f, 0.0f, 1.0f});
		_engine->clear(1.0f);

		// View on the scene
		auto cbuf_temp = _engine->requestPerFrameConstantBuffer<ColourTemperature>();
		if (_animate)
		{
			cbuf_temp->Temperature = 1000 + (5500 - 1000) * _animation_value;
			cbuf_temp->Value = 0.5f + 0.5f * _animation_value;
		}
		else
		{
			cbuf_temp->Temperature = 1000 + (5500 - 1000) * _colour_temperature;
			cbuf_temp->Value = 0.5f + 0.5f * _colour_value;
		}
		_engine->setConstantBuffer(0, cbuf_temp);

		renderScene(Vcl::Graphics::Runtime::PrimitiveType::Trianglelist, _engine.get(), _temperaturePS);
		
		_engine->endFrame();
	}
	
	bool animate() const { return _animate; }
	void setAnimate(bool a) { _animate = a; }

	float colourTemperatur() const { return _colour_temperature; }
	void setColourTemperatur(float t) { _colour_temperature = t; }

	float colourValue() const { return _colour_value; }
	void setColourValue(float v) { _colour_value = v; }

private:
	void renderScene
	(
		Vcl::Graphics::Runtime::PrimitiveType primitive_type,
		gsl::not_null<Vcl::Graphics::Runtime::GraphicsEngine*> cmd_queue,
		Vcl::ref_ptr<Vcl::Graphics::Runtime::PipelineState> ps
	)
	{
		// Configure the layout
		cmd_queue->setPipelineState(ps);

		// Render the quad
		cmd_queue->setPrimitiveType(primitive_type, 3);
		cmd_queue->draw(6);
	}

private:
	std::unique_ptr<Vcl::Graphics::Runtime::GraphicsEngine> _engine;

private:
	std::unique_ptr<Vcl::Graphics::TrackballCameraController> _cameraController;

private:
	std::unique_ptr<Vcl::Graphics::Camera> _camera;

	//! Automatic animation
	bool _animate{ false };
	
	//! Animation value
	float _animation_value{ 0 };

	//! Interpolation timespan
	std::chrono::seconds _animation_length{ 10 };

	//! Last time-stamp
	std::chrono::nanoseconds _last_check{ 0 };

	//! Colour temperature
	float _colour_temperature{ 0 };

	//! Colour value
	float _colour_value{ 1 };

	std::unique_ptr<Vcl::Graphics::Runtime::OpenGL::PipelineState> _temperaturePS;
};

VCL_RTTI_BASES(WrinkledSurfacesExample, BaseScene)

VCL_RTTI_CTOR_TABLE_BEGIN(WrinkledSurfacesExample)
	Vcl::RTTI::Constructor<WrinkledSurfacesExample>()
VCL_RTTI_CTOR_TABLE_END(WrinkledSurfacesExample)

VCL_RTTI_ATTR_TABLE_BEGIN(WrinkledSurfacesExample)
	Vcl::RTTI::Attribute<WrinkledSurfacesExample, bool>{ "Animate", &WrinkledSurfacesExample::animate, &WrinkledSurfacesExample::setAnimate },
	Vcl::RTTI::Attribute<WrinkledSurfacesExample, float>{ "ColourTemperature", &WrinkledSurfacesExample::colourTemperatur, &WrinkledSurfacesExample::setColourTemperatur },
	Vcl::RTTI::Attribute<WrinkledSurfacesExample, float>{ "ColourValue", &WrinkledSurfacesExample::colourValue, &WrinkledSurfacesExample::setColourValue }
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
	app.setSceneDrawCallback([&scene](Application& app) {scene.draw(app);});
	app.setUIDrawCallback([&scene](Application& app) {scene.drawUI(app);});
	
	return app.run();
}
