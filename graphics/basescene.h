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
#pragma once

// Abseil
#include <absl/strings/string_view.h>

// GLFW
#include <GLFW/glfw3.h>

// ImGui
#include <imgui.h>

class BaseScene
{
	VCL_DECLARE_ROOT_METAOBJECT(BaseScene)

public:
	virtual void drawUI(Application& app)
	{
		show(*this);
	}

	virtual void draw(Application& app) = 0;

protected:

	void show(BaseScene& obj)
	{
		ImGuiWindowFlags corner =
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoTitleBar;

		ImGui::Begin("Configuration", nullptr, corner);
		ImGui::SetWindowPos({ 10, 10 });

		const auto* type = obj.metaType();
		for (const auto* attr : type->attributes())
		{
			std::any value;
			attr->get(&obj, value);

			if (value.type() == typeid(bool))
			{
				auto* ui_value = std::any_cast<bool>(&value);
				if (ImGui::Checkbox(attr->name().data(), ui_value))
				{
					attr->set(&obj, value);
				}
			}
			else if (value.type() == typeid(float))
			{
				auto* ui_value = std::any_cast<float>(&value);
				if (ImGui::InputFloat(attr->name().data(), ui_value))
				{
					attr->set(&obj, value);
				}
			}
		}
		ImGui::End();
	}
};
Vcl::RTTI::ConstructableType<BaseScene> type{ "BaseScene", sizeof(BaseScene), std::alignment_of<BaseScene>::value };
VCL_DEFINE_METAOBJECT(BaseScene)
{
}
