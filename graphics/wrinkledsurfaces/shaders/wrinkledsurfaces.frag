/*
 * This file is part of the Visual Computing Library (VCL) release under the
 * MIT license.
 *
 * Copyright (c) 2017 Basil Fierz
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
#version 450 core
#extension GL_ARB_enhanced_layouts : enable

#include "wrinkledsurfaces.h"
#include "perturbnormal.glsl"

////////////////////////////////////////////////////////////////////////////////
// Shader Input
////////////////////////////////////////////////////////////////////////////////
layout(location = 0) in VertexData
{
	// View space position
	vec3 Position;

	// View space surface normal
	vec3 Normal;

	// View space surface tangent
	vec3 Tangent;

	// 2D surface parameterization
	vec2 TexCoords;
} In;

////////////////////////////////////////////////////////////////////////////////
// Shader Output
////////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 FragColour;

////////////////////////////////////////////////////////////////////////////////
// Shader Constants
////////////////////////////////////////////////////////////////////////////////
layout(location = 0) uniform sampler2D DiffuseMap;
layout(location = 1) uniform sampler2D HeightMap;
layout(location = 2) uniform sampler2D DerivativeMap;
layout(location = 3) uniform sampler2D NormalObjMap;
layout(location = 4) uniform sampler2D NormalTanMap;

////////////////////////////////////////////////////////////////////////////////
// Specialization Constants
////////////////////////////////////////////////////////////////////////////////
#ifdef GL_SPIRV
layout(constant_id = 0) const uint DetailMode = 0;
layout(location = 5) uniform uint DetailModeUniform = 0;
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
	const vec3 albedo = texture(DiffuseMap, In.TexCoords).rgb;
	const vec3 specular = vec3(1);
	const float shininess = 16;

	// Different ways to determine the surface normal
	vec3 N = In.Normal;
	switch (DetailModeUniform)
	{
	case 1: // Object space normal mapping
	{
		vec3 N_obj = 2 * texture(NormalObjMap, In.TexCoords).xyz - 1;
		N = normalize(mat3(NormalMatrix) * N_obj);
		break;
	}	
	case 2: // Tangents space normal mapping
	{
		vec3 N_ts = 2 * texture(NormalTanMap, In.TexCoords).xyz - 1;

		vec3 T_vs, B_vs, N_ws;
		T_vs = In.Tangent;
		B_vs = cross(N, T_vs);
		N_ws = mat3(T_vs, B_vs, N) * N_ts;

		N = normalize(N_ws);
		break;
	}
	case 3: // Pertubated normals
	case 4: // Screen space normals for displacement mapping
	{
		//N = perturbNormal(In.Position, N, dHdxyDirect(HeightMap, In.TexCoords, 0.1));
		N = perturbNormal(In.Position, N, dHdxyDerivatives(DerivativeMap, In.TexCoords));
		break;
	}
	}

	const vec3 view_dir = -normalize(In.Position);
	const vec3 light_dir = normalize(In.Position - (ViewMatrix * point_light).xyz);

	float diff_refl = max(0, dot(N, -light_dir));
	float spec_refl = pow(max(0, dot(reflect(-light_dir, N), -view_dir)), shininess);

	FragColour = vec4(albedo*diff_refl + specular*spec_refl, 1);
}
