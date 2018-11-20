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

// VCL
#include <vcl/rtti/metatype.h>
#include <vcl/rtti/metatypeconstructor.inl>

struct Colour3f
{
	float r, g, b;
};

namespace Vcl
{
	template<>
	inline std::string to_string<Colour3f>(const Colour3f& value)
	{
		std::stringstream ss;

		ss << std::to_string(value.r);
		ss << ", ";
		ss << std::to_string(value.g);
		ss << ", ";
		ss << std::to_string(value.b);

		return ss.str();
	}

	template<>
	inline Colour3f from_string<Colour3f>(const std::string& value)
	{
		size_t pos = 0;
		size_t next = 0;
		const float v0 = std::stof(value, &next);             pos += next;
		const float v1 = std::stof(value.substr(pos), &next); pos += next;
		const float v2 = std::stof(value.substr(pos));

		return { v0, v1, v2 };
	}
}

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
			if (attr->isEnum())
			{
				auto* enum_attr = static_cast<const Vcl::RTTI::EnumAttributeBase*>(attr);
				const auto nr_enums = enum_attr->count();

				std::string value;
				attr->get(&obj, value);

				static ImGuiComboFlags flags = 0;
				if (ImGui::BeginCombo(attr->name().data(), value.c_str(), flags))
				{
					std::string old_value = value;
					for (uint32_t n = 0; n < nr_enums; n++)
					{
						const auto item = enum_attr->enumName(n);
						bool is_selected = (value == item);
						if (ImGui::Selectable(item.c_str(), is_selected))
							value = item;
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();

					if (value != old_value)
						attr->set(&obj, value);
				}
			}
			else
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
				else if (value.type() == typeid(Colour3f))
				{
					auto* ui_value = std::any_cast<Colour3f>(&value);
					if (ImGui::ColorEdit3(attr->name().data(), reinterpret_cast<float*>(ui_value)))
					{
						attr->set(&obj, value);
					}
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
