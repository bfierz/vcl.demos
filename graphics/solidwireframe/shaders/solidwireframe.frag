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
#version 430 core
#extension GL_ARB_enhanced_layouts : enable

#include "solidwireframe.h"
#include "solidwireframe.glsl"

////////////////////////////////////////////////////////////////////////////////
// Shader Input
////////////////////////////////////////////////////////////////////////////////
layout(location = 0) in VertexData
{
	// View space position
	vec3 Position;

	// View space surface normal
	vec3 Normal;
	
	// Barycentric coordinates
	vec2 BarycentricCoords;
} In;

////////////////////////////////////////////////////////////////////////////////
// Shader Output
////////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 FragColour;

////////////////////////////////////////////////////////////////////////////////
// Shader Constants
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Specialization Constants
////////////////////////////////////////////////////////////////////////////////
#ifdef GL_SPIRV
#else
#	error "Require SPIR-V support"
#endif

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////

// Light position (point-light)
const vec4 point_light = vec4(-1, 0, 1, 1);

void main(void)
{
	vec3 albedo = vec3(0.7f, 0.7f, 0.7f);
	vec3 specular = vec3(0.2f, 0.2f, 0.2f);
	float shininess = 32;

	vec3 baryc;
	baryc.xy = In.BarycentricCoords;
	baryc.z = 1 - In.BarycentricCoords.x - In.BarycentricCoords.y;
	albedo = solidWireframe(baryc, albedo, Smoothing, Thickness, Colour);

	vec3 N = In.Normal;

	const vec3 view_dir = -normalize(In.Position);
	const vec3 light_dir = normalize(In.Position - (ViewMatrix * point_light).xyz);

	const float diff_refl = max(0, dot(N, -light_dir));
	const float spec_refl = pow(max(0, dot(reflect(-light_dir, N), -view_dir)), shininess);

	FragColour = vec4(albedo*diff_refl + specular*spec_refl, 1);
}
