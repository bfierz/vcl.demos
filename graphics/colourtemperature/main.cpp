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
#include <chrono>
#include <iostream>

// GSL
#include <gsl/gsl>

// NanoGUI
#include <nanogui/checkbox.h>
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

#include "shaders/temperature.h"
#include "temperature.vert.spv.h"
#include "temperature.frag.spv.h"

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

		Window *window = new Window(this, "Colour Settings");
		window->setPosition(Vector2i(15, 15));
		window->setLayout(new GroupLayout());

		Widget *panel = new Widget(window);
		panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));
		
		new Label(panel, "Colour Temperature", "sans-bold");
		Slider *slider_temp = new Slider(panel);
		slider_temp->setValue(_colour_temperature);
		slider_temp->setFixedSize(Vector2i(60, 25));
		slider_temp->setCallback([this](float val) -> bool
		{
			_colour_temperature = val;
			return true;
		});
		
		new Label(panel, "Colour Value", "sans-bold");
		Slider *slider_val= new Slider(panel);
		slider_val->setValue(_colour_value);
		slider_val->setFixedSize(Vector2i(60, 25));
		slider_val->setCallback([this](float val) -> bool
		{
			_colour_value = val;
			return true;
		});
		
		auto auto_box = new CheckBox(panel, "Auto", [this](bool val) -> bool
		{
			_animate = val;
			if (val)
			{
				// Start a new animation timer
				_animation_value = 0;
				_last_check = std::chrono::high_resolution_clock::now().time_since_epoch();
			}
			return true;
		});
		auto_box->setChecked(_animate);
		
		performLayout();

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
	void drawContents() override
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
