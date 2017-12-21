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
layout(vertices = 3) out;

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
} Out[];

////////////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////////// 
void main()
{
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	Out[gl_InvocationID].Position  = In[gl_InvocationID].Position;
	Out[gl_InvocationID].Normal    = In[gl_InvocationID].Normal;
	Out[gl_InvocationID].Tangent   = In[gl_InvocationID].Tangent;
	Out[gl_InvocationID].TexCoords = In[gl_InvocationID].TexCoords;

	if (gl_InvocationID == 0)
	{
		gl_TessLevelInner[0] = Level;
		gl_TessLevelOuter[0] = Level;
		gl_TessLevelOuter[1] = Level;
		gl_TessLevelOuter[2] = Level;
	}
}
