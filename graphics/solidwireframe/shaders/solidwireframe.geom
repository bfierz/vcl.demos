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

////////////////////////////////////////////////////////////////////////////////
// Shader Configuration
////////////////////////////////////////////////////////////////////////////////
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

////////////////////////////////////////////////////////////////////////////////
// Shader Input
////////////////////////////////////////////////////////////////////////////////
layout(location = 0) in VertexData
{
	// View space position
	vec3 Position;

	// View space surface normal
	vec3 Normal;
} In[3];

////////////////////////////////////////////////////////////////////////////////
// Shader Output
////////////////////////////////////////////////////////////////////////////////
layout(location = 0) out VertexData
{
	// View space position
	vec3 Position;

	// View space surface normal
	vec3 Normal;

	// Barycentric coordinates
	vec2 BarycentricCoords;
} Out;

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////
void main()
{
	vec3 p0 = In[0].Position;
	vec3 p1 = In[1].Position;
	vec3 p2 = In[2].Position;
	vec3 n0 = In[0].Normal;
	vec3 n1 = In[1].Normal;
	vec3 n2 = In[2].Normal;

	vec3 face_normal = normalize(cross(p1 - p0, p2 - p0));
	n0 = face_normal;
	n1 = face_normal;
	n2 = face_normal;

	Out.Position = p0; Out.Normal = n0; Out.BarycentricCoords = vec2(1, 0); gl_Position = ProjectionMatrix * vec4(p0, 1); EmitVertex();
	Out.Position = p1; Out.Normal = n1; Out.BarycentricCoords = vec2(0, 1); gl_Position = ProjectionMatrix * vec4(p1, 1); EmitVertex();
	Out.Position = p2; Out.Normal = n2; Out.BarycentricCoords = vec2(0, 0); gl_Position = ProjectionMatrix * vec4(p2, 1); EmitVertex();
	EndPrimitive();
}
