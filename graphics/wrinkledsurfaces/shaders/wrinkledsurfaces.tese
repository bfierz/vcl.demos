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
// Shader Configuration
////////////////////////////////////////////////////////////////////////////////
layout(triangles) in;

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
} In[];

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
// Shader Constants
////////////////////////////////////////////////////////////////////////////////
layout(location = 1) uniform sampler2D HeightMap;

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////
void main()
{
	vec3 p0 = gl_TessCoord.x * In[0].Position.xyz;
	vec3 p1 = gl_TessCoord.y * In[1].Position.xyz;
	vec3 p2 = gl_TessCoord.z * In[2].Position.xyz;
	vec3 pos = p0 + p1 + p2;

	vec3 n0 = gl_TessCoord.x * In[0].Normal;
	vec3 n1 = gl_TessCoord.y * In[1].Normal;
	vec3 n2 = gl_TessCoord.z * In[2].Normal;
	vec3 normal = normalize(n0 + n1 + n2);
	Out.Normal = normal;
	
	vec3 t0 = gl_TessCoord.x * In[0].Tangent;
	vec3 t1 = gl_TessCoord.y * In[1].Tangent;
	vec3 t2 = gl_TessCoord.z * In[2].Tangent;
	vec3 tangent = normalize(t0 + t1 + t2);
	Out.Tangent = tangent;

	vec2 tc0 = gl_TessCoord.x * In[0].TexCoords;
	vec2 tc1 = gl_TessCoord.y * In[1].TexCoords;
	vec2 tc2 = gl_TessCoord.z * In[2].TexCoords;  
	vec2 tc = tc0 + tc1 + tc2;
	Out.TexCoords = tc;
		
	float height = 2 * (texture(HeightMap, tc).x - Midlevel);
	vec3 disp = normal * HeightScale * height;
	
	Out.Position = pos;
	gl_Position = ProjectionMatrix * vec4(pos + disp, 1);
}
