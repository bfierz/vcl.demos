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
#ifndef GLSL_SOLIDWIREFRAME
#define GLSL_SOLIDWIREFRAME

// Generate a solid wireframe based on the barycentric coordinates of the
// triangle. Base concept can be found here:
// https://catlikecoding.com/unity/tutorials/advanced-rendering/flat-and-wireframe-shading/
vec3 solidWireframe
(
	vec3 baryc, vec3 ground_colour,
	float smoothing, float thickness, vec3 wireframe_colour
)
{
	vec3 deltas = fwidth(baryc);
	vec3 s = deltas * smoothing;
	vec3 t = deltas * thickness;
	baryc = smoothstep(t, t+ s, baryc);

	float min_baryc = min(baryc.x, min(baryc.y, baryc.z));
	return mix(wireframe_colour, ground_colour, min_baryc);
}

#endif // GLSL_SOLIDWIREFRAME
