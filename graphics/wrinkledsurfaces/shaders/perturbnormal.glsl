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
#ifndef GLSL_PERTURBNORMAL
#define GLSL_PERTURBNORMAL

// Derivative maps - bump mapping unparametrized surfaces by Morten Mikkelsen
// http://mmikkelsen3d.blogspot.sk/2011/07/derivative-maps.html

// Some additional resources
// http://trevorius.com/scrapbook/uncategorized/derivative-normal-maps/
// http://www.rorydriscoll.com/2012/01/11/derivative-maps/
// http://www.rorydriscoll.com/2012/01/15/derivative-maps-vs-normal-maps/
// http://therobotseven.com/devlog/triplanar-texturing-derivative-maps/
// http://polycount.com/discussion/91605/derivative-normal-maps-what-are-they
// https://docs.knaldtech.com/doku.php?id=derivative_maps_knald

// Evaluate the derivative of the height w.r.t. screen-space using forward differencing (listing 2)
vec2 dHdxyFwd(sampler2D bump_map, vec2 uv, float scale)
{
	vec2 dSTdx = dFdxFine(uv);
	vec2 dSTdy = dFdyFine(uv);

	float h   = scale * texture(bump_map, uv).x;
	float dBx = scale * texture(bump_map, uv + dSTdx).x - h;
	float dBy = scale * texture(bump_map, uv + dSTdy).x - h;

	return vec2(dBx, dBy);
}
vec2 dHdxyDirect(sampler2D bump_map, vec2 uv, float scale)
{
	float h = scale * texture(bump_map, uv).x;
	float dBx = dFdxFine(h);
	float dBy = dFdyFine(h);

	return vec2(dBx, dBy);
}

// Evaluate the height derivatives using a preomputed map of partial derivatives
vec2 dHdxyDerivatives(sampler2D derivative_map, vec2 uv)
{
	vec2 dSTdx = dFdxFine(uv);
	vec2 dSTdy = dFdyFine(uv);
	vec2 dBdst = 2 * texture(derivative_map, uv).xy - vec2(1);
	float dBx = dBdst.x * dSTdx.x + dBdst.y * dSTdx.y;
	float dBy = dBdst.x * dSTdy.x + dBdst.y * dSTdy.y;

	return vec2(dBx, dBy);
}

// Surface normal is normalized
vec3 perturbNormal(vec3 surf_pos, vec3 surf_norm, vec2 dHdxy)
{
	vec3 SigmaX = dFdxFine(surf_pos);
	vec3 SigmaY = dFdyFine(surf_pos);
	vec3 N = surf_norm;

	vec3 R1 = cross(SigmaY, N);
	vec3 R2 = cross(N, SigmaX);

	float det = dot(SigmaX, R1);

	vec3 grad = sign(det) * (dHdxy.x * R1 + dHdxy.y * R2);
	return normalize(abs(det) * N - grad);
}

#endif // GLSL_PERTURBNORMAL
