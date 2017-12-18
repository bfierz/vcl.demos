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
#version 430 core
#extension GL_ARB_enhanced_layouts : enable

#include "wrinkledsurfaces.h"

////////////////////////////////////////////////////////////////////////////////
// Shader Output
////////////////////////////////////////////////////////////////////////////////
layout(location = 0) out VertexData
{
	// View space position
	vec3 Position;

	// View space surface normal
	vec3 Normal;

	// View space surface tangent
	vec3 Tangent;

	// 2D surface parameterization
	vec2 TexCoords;
} Out;

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////
const vec3 vertices[] = {
	vec3(-1, -1, 0),
	vec3( 1, -1, 0),
	vec3(-1,  1, 0),
	vec3( 1,  1, 0)
};
const vec2 tex_coords[] = {
	vec2(0, 1),
	vec2(1, 1),
	vec2(0, 0),
	vec2(1, 0)
};

void main()
{
	// Vertex index in the loop
	int node_id = gl_VertexID % 4;

	vec4 pos_vs = ViewMatrix * ModelMatrix * vec4(vertices[node_id], 1);

	// Pass data
	Out.Position  = pos_vs.xyz;
	Out.Normal    = (NormalMatrix * vec4(0, 0, 1, 0)).xyz;
	Out.Tangent   = (NormalMatrix * vec4(1, 0, 0, 0)).xyz;
	Out.TexCoords = tex_coords[node_id];

	// Transform the point to view space
	gl_Position = ProjectionMatrix * pos_vs;
}
