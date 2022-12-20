/* Copyright (c) 2020 Themaister
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define NOMINMAX
#include "triangle_converter.hpp"
#include <utility>
#include <algorithm>
#include <cmath>
#include <limits>
#include <assert.h>

// A very straight forward implementation of a triangle clipper and setup.
// It is not optimized at all.

namespace RDP
{
static int16_t clamp_float_int16(float v)
{
	if (v < float(-0x8000))
		return -0x8000;
	else if (v > float(0x7fff))
		return 0x7fff;
	else
		return int16_t(v);
}

static int16_t quantize_y(float x)
{
	x *= float(1 << SUBPIXELS_LOG2);
	return clamp_float_int16(std::round(x));
}

static int16_t quantize_x(float x)
{
	x *= float(1 << SUBPIXELS_LOG2);
	return clamp_float_int16(std::round(x));
}

static int32_t quantize_color(double c)
{
	double rounded = std::round(c * 255.0 * double(1 << 16));
	return int32_t(rounded);
}

static int32_t quantize_u(double c)
{
	double rounded = std::round(c * double(1 << 6) * double(1 << 16));
	return int32_t(rounded);
}

static int32_t quantize_v(double c)
{
	double rounded = std::round(c * double(1 << 6) * double(1 << 16));
	return int32_t(rounded);
}

static int32_t quantize_w(double c)
{
	double rounded = std::round(c * double(1ll << 32));
	return int32_t(rounded);
}

static int32_t quantize_z(double z)
{
	double rounded = std::round(z * double((1 << 18) - 1) * double(1 << 13));
	return int32_t(rounded);
}

#if 0
static int32_t quantize_z(float z)
{
	float rounded = std::round(z * float(((1 << 16) - 1) << 8));
	assert(rounded <= float(std::numeric_limits<int32_t>::max()));
	return int32_t(rounded);
}

static int32_t quantize_bary(float z)
{
	float rounded = std::round(z * float(1 << 16));
	assert(rounded <= float(std::numeric_limits<int32_t>::max()));
	return int32_t(rounded);
}

static int32_t quantize_w(float w)
{
	float rounded = std::round(w * float(1 << 16));
	assert(rounded <= float(std::numeric_limits<int32_t>::max()));
	return int32_t(rounded);
}

static int32_t quantize_uv(float v)
{
	float rounded = std::round(v * float(1 << 16));
	assert(rounded <= float(std::numeric_limits<int32_t>::max()));
	return int32_t(rounded);
}
#endif

static int32_t round_away_from_zero_divide(int32_t x, int32_t y)
{
	int32_t rounding = y - 1;
	if (x < 0)
		x -= rounding;
	else if (x > 0)
		x += rounding;

	return x / y;
}

static bool setup_triangle(PrimitiveSetup &setup, const InputPrimitive &input, CullMode cull_mode)
{
	setup = {};

	const int16_t xs[] = { quantize_x(input.vertices[0].x), quantize_x(input.vertices[1].x), quantize_x(input.vertices[2].x) };
	const int16_t ys[] = { quantize_y(input.vertices[0].y), quantize_y(input.vertices[1].y), quantize_y(input.vertices[2].y) };

	int index_a = 0;
	int index_b = 1;
	int index_c = 2;

	// Sort primitives by height, tie break by sorting on X.
	if (ys[index_b] < ys[index_a])
		std::swap(index_b, index_a);
	else if (ys[index_b] == ys[index_a] && xs[index_b] < xs[index_a])
		std::swap(index_b, index_a);

	if (ys[index_c] < ys[index_b])
		std::swap(index_c, index_b);
	else if (ys[index_c] == ys[index_b] && xs[index_c] < xs[index_b])
		std::swap(index_c, index_b);

	if (ys[index_b] < ys[index_a])
		std::swap(index_b, index_a);
	else if (ys[index_b] == ys[index_a] && xs[index_b] < xs[index_a])
		std::swap(index_b, index_a);

	int16_t y_lo = ys[index_a];
	int16_t y_mid = ys[index_b];
	int16_t y_hi = ys[index_c];

	int16_t x_a = xs[index_a];
	int16_t x_b = xs[index_b];
	int16_t x_c = xs[index_c];

	setup.pos.x_a = x_a << (16 - SUBPIXELS_LOG2);
	setup.pos.x_b = x_a << (16 - SUBPIXELS_LOG2);
	setup.pos.x_c = x_b << (16 - SUBPIXELS_LOG2);

	setup.pos.y_lo = y_lo;
	setup.pos.y_mid = y_mid;
	setup.pos.y_hi = y_hi;

	// Compute slopes.
	// Not sure if specific rounding away from zero is actually required,
	// but I've seen it in a few implementations.
	setup.pos.dxdy_a = round_away_from_zero_divide((x_c - x_a) << 16, std::max(1, y_hi - y_lo));
	setup.pos.dxdy_b = round_away_from_zero_divide((x_b - x_a) << 16, std::max(1, y_mid - y_lo));
	setup.pos.dxdy_c = round_away_from_zero_divide((x_c - x_b) << 16, std::max(1, y_hi - y_mid));

	// These bits are ignored in rasterizer.
	setup.pos.dxdy_a &= ~7;
	setup.pos.dxdy_b &= ~7;
	setup.pos.dxdy_c &= ~7;

	// Stepping begins from integer Y on the two first slopes. Fix this up now.
	unsigned sub_pix_y = y_lo & ((1 << SUBPIXELS_LOG2) - 1);
	setup.pos.x_a -= (setup.pos.dxdy_a >> SUBPIXELS_LOG2) * sub_pix_y;
	setup.pos.x_b -= (setup.pos.dxdy_b >> SUBPIXELS_LOG2) * sub_pix_y;

	if (setup.pos.dxdy_b < setup.pos.dxdy_a)
		setup.pos.flags |= PRIMITIVE_RIGHT_MAJOR_BIT;

	// Compute winding before reorder.
	int ab_x = xs[1] - xs[0];
	int ab_y = ys[1] - ys[0];
	int bc_x = xs[2] - xs[1];
	int bc_y = ys[2] - ys[1];
	int ca_x = xs[0] - xs[2];
	int ca_y = ys[0] - ys[2];

	// Standard cross product.
	int signed_area = ab_x * bc_y - ab_y * bc_x;

	// Check if triangle is degenerate or we can cull it based on winding.
	if (signed_area == 0)
		return false;
	else if (cull_mode == CullMode::CCWOnly && signed_area > 0)
		return false;
	else if (cull_mode == CullMode::CWOnly && signed_area < 0)
		return false;

	// Recompute based on reordered vertices, so we get correct interpolation equations.
	ab_x = x_b - x_a;
	bc_x = x_c - x_b;
	ca_x = x_a - x_c;
	ab_y = y_mid - y_lo;
	bc_y = y_hi - y_mid;
	ca_y = y_lo - y_hi;

	// Standard cross product.
	signed_area = ab_x * bc_y - ab_y * bc_x;

	double inv_signed_area = double(1 << SUBPIXELS_LOG2) / float(signed_area);

	double dxdy = double(setup.pos.dxdy_a) / double(0x10000);
	double yfrac = double(y_lo & ((1 << SUBPIXELS_LOG2) - 1)) / double(1 << SUBPIXELS_LOG2);

	for (unsigned c = 0; c < 4; c++)
	{
		double dcdx = -inv_signed_area * (double(ab_y) * input.vertices[index_c].color[c] +
		                                 double(ca_y) * input.vertices[index_b].color[c] +
		                                 double(bc_y) * input.vertices[index_a].color[c]);

		double dcdy = inv_signed_area * (double(ab_x) * input.vertices[index_c].color[c] +
		                                double(ca_x) * input.vertices[index_b].color[c] +
		                                double(bc_x) * input.vertices[index_a].color[c]);

		// For some reason the RDP has three equations here.
		double dcde = dcdy + dcdx * dxdy;

		double color = input.vertices[index_a].color[c];

		// Fixup for interpolation. Interpolation is assumed to begin from the integer portion of X and Y.
		color -= yfrac * dcde;

		setup.attr.c[c] = quantize_color(color);
		setup.attr.dcdx[c] = quantize_color(dcdx);
		setup.attr.dcdy[c] = quantize_color(dcdy);
		setup.attr.dcde[c] = quantize_color(dcde);
	}

#define COMPUTE_SLOPES(a) \
	double a = input.vertices[index_a].a; \
	double d##a##dx = -inv_signed_area * (double(ab_y) * input.vertices[index_c].a + \
	                                 double(ca_y) * input.vertices[index_b].a + \
	                                 double(bc_y) * input.vertices[index_a].a); \
	double d##a##dy = inv_signed_area * (double(ab_x) * input.vertices[index_c].a + \
	                                double(ca_x) * input.vertices[index_b].a + \
	                                double(bc_x) * input.vertices[index_a].a); \
	double d##a##de = d##a##dy + d##a##dx * dxdy; \
	a -= yfrac * d##a##de; \
	setup.attr.a = quantize_##a(a); \
	setup.attr.d##a##dx = quantize_##a(d##a##dx); \
	setup.attr.d##a##de = quantize_##a(d##a##de); \
	setup.attr.d##a##dy = quantize_##a(d##a##dy)

	COMPUTE_SLOPES(z);
	COMPUTE_SLOPES(u);
	COMPUTE_SLOPES(v);
	COMPUTE_SLOPES(w);

	setup.pos.flags |= PRIMITIVE_PERSPECTIVE_CORRECT_BIT;
	return true;
}

static void interpolate_vertex(Vertex &v, const Vertex &a, const Vertex &b, float l)
{
	float left = 1.0f - l;
	float right = l;

	for (int i = 0; i < 4; i++)
	{
		v.clip[i] = a.clip[i] * left + b.clip[i] * right;
		v.color[i] = a.color[i] * left + b.color[i] * right;
	}

	v.u = a.u * left + b.u * right;
	v.v = a.v * left + b.v * right;
}

// Create a bitmask for which vertices clip outside some boundary.
static unsigned get_clip_code_low(const InputPrimitive &prim, float limit, unsigned comp)
{
	bool clip_a = prim.vertices[0].clip[comp] < limit;
	bool clip_b = prim.vertices[1].clip[comp] < limit;
	bool clip_c = prim.vertices[2].clip[comp] < limit;
	unsigned clip_code = (unsigned(clip_a) << 0) | (unsigned(clip_b) << 1) | (unsigned(clip_c) << 2);
	return clip_code;
}

// Create a bitmask for which vertices clip outside some boundary.
static unsigned get_clip_code_high(const InputPrimitive &prim, float limit, unsigned comp)
{
	bool clip_a = prim.vertices[0].clip[comp] > limit;
	bool clip_b = prim.vertices[1].clip[comp] > limit;
	bool clip_c = prim.vertices[2].clip[comp] > limit;
	unsigned clip_code = (unsigned(clip_a) << 0) | (unsigned(clip_b) << 1) | (unsigned(clip_c) << 2);
	return clip_code;
}

// Interpolates two vertices towards one vertex which is inside the clip region.
// No new vertices are generated.
static void clip_single_output(InputPrimitive &output, const InputPrimitive &input, unsigned component, float target,
                               unsigned a, unsigned b, unsigned c)
{
	float interpolate_a = (target - input.vertices[a].clip[component]) /
	                      (input.vertices[c].clip[component] - input.vertices[a].clip[component]);
	float interpolate_b = (target - input.vertices[b].clip[component]) /
	                      (input.vertices[c].clip[component] - input.vertices[b].clip[component]);

	interpolate_vertex(output.vertices[a], input.vertices[a], input.vertices[c], interpolate_a);
	interpolate_vertex(output.vertices[b], input.vertices[b], input.vertices[c], interpolate_b);

	// To avoid precision issues in interpolating, we expect the new vertex to be perfectly aligned with the clip plane.
	output.vertices[a].clip[component] = target;
	output.vertices[b].clip[component] = target;

	output.vertices[c] = input.vertices[c];
}

// Interpolate one vertex against the clip plane.
// This creates two primitives, not one.
static void clip_dual_output(InputPrimitive *output, const InputPrimitive &input, unsigned component, float target,
                             unsigned a, unsigned b, unsigned c)
{
	float interpolate_ab = (target - input.vertices[a].clip[component]) /
	                       (input.vertices[b].clip[component] - input.vertices[a].clip[component]);
	float interpolate_ac = (target - input.vertices[a].clip[component]) /
	                       (input.vertices[c].clip[component] - input.vertices[a].clip[component]);

	Vertex ab, ac;
	interpolate_vertex(ab, input.vertices[a], input.vertices[b], interpolate_ab);
	interpolate_vertex(ac, input.vertices[a], input.vertices[c], interpolate_ac);

	// To avoid precision issues in interpolating, we expect the new vertex to be perfectly aligned with the clip plane.
	ab.clip[component] = target;
	ac.clip[component] = target;

	output[0].vertices[0] = ab;
	output[0].vertices[1] = input.vertices[b];
	output[0].vertices[2] = ac;
	output[1].vertices[0] = ac;
	output[1].vertices[1] = input.vertices[b];
	output[1].vertices[2] = input.vertices[c];
}

// Clipping a primitive results in 0, 1 or 2 primitives.
static unsigned clip_component(InputPrimitive *prims, const InputPrimitive &prim, unsigned component,
                               float target, unsigned code)
{
	switch (code)
	{
	case 0:
		// Nothing to clip. 1:1
		prims[0] = prim;
		return 1;

	case 1:
		// Clip vertex A. 2 new primitives.
		clip_dual_output(prims, prim, component, target, 0, 1, 2);
		return 2;

	case 2:
		// Clip vertex B. 2 new primitives.
		clip_dual_output(prims, prim, component, target, 1, 2, 0);
		return 2;

	case 3:
		// Interpolate A and B against C. 1 primitive.
		clip_single_output(prims[0], prim, component, target, 0, 1, 2);
		return 1;

	case 4:
		// Clip vertex C. 2 new primitives.
		clip_dual_output(prims, prim, component, target, 2, 0, 1);
		return 2;

	case 5:
		// Interpolate A and C against B. 1 primitive.
		clip_single_output(prims[0], prim, component, target, 2, 0, 1);
		return 1;

	case 6:
		// Interpolate B and C against A. 1 primitive.
		clip_single_output(prims[0], prim, component, target, 1, 2, 0);
		return 1;

	case 7:
		// All clipped. Discard primitive.
		return 0;

	default:
		return 0;
	}
}

static unsigned clip_triangles(InputPrimitive *outputs, const InputPrimitive *inputs, unsigned count, unsigned component, float target)
{
	unsigned output_count = 0;

	for (unsigned i = 0; i < count; i++)
	{
		unsigned clip_code;
		if (target > 0.0f)
			clip_code = get_clip_code_high(inputs[i], target, component);
		else
			clip_code = get_clip_code_low(inputs[i], target, component);

		unsigned clipped_count = clip_component(outputs, inputs[i], component, target, clip_code);
		output_count += clipped_count;
		outputs += clipped_count;
	}

	return output_count;
}

static unsigned setup_clipped_triangles_clipped_w(PrimitiveSetup *setup, InputPrimitive &prim, CullMode mode, const ViewportTransform &vp)
{
	// Cull primitives on X/Y early.
	// If all vertices are outside clip-space, we know the primitive is not visible.
	if (prim.vertices[0].x < -prim.vertices[0].w &&
	    prim.vertices[1].x < -prim.vertices[1].w &&
	    prim.vertices[2].x < -prim.vertices[2].w)
	{
		return 0;
	}
	else if (prim.vertices[0].y < -prim.vertices[0].w &&
	         prim.vertices[1].y < -prim.vertices[1].w &&
	         prim.vertices[2].y < -prim.vertices[2].w)
	{
		return 0;
	}
	else if (prim.vertices[0].x > prim.vertices[0].w &&
	         prim.vertices[1].x > prim.vertices[1].w &&
	         prim.vertices[2].x > prim.vertices[2].w)
	{
		return 0;
	}
	else if (prim.vertices[0].y > prim.vertices[0].w &&
	         prim.vertices[1].y > prim.vertices[1].w &&
	         prim.vertices[2].y > prim.vertices[2].w)
	{
		return 0;
	}

	// FIXME: Not sure what the theoretical bound is, but it's probably way less than 256.
	InputPrimitive tmp_a[256];
	InputPrimitive tmp_b[256];

#if 1
	// Fixed point consideration.
	const float ws[3] = {
		prim.vertices[0].w,
		prim.vertices[1].w,
		prim.vertices[2].w,
	};

	float min_w = std::numeric_limits<float>::max();
	for (auto w : ws)
		min_w = std::min(min_w, w);
	// Make sure to fit in fixed point format for W.
	min_w *= 0.49f;
#endif

#if 0
	// Try to center UV coordinates close to 0 for better division precision.
	// This makes more sense for fixed point interpolation than FP interpolation though ...
	float u_offset = floorf((1.0f / 3.0f) * (prim.vertices[0].u + prim.vertices[1].u + prim.vertices[2].u));
	float v_offset = floorf((1.0f / 3.0f) * (prim.vertices[0].v + prim.vertices[1].v + prim.vertices[2].v));
#endif

	// Perform perspective divide here, and replace W with 1/W.
	// This allows us to perform perspective correct clipping without
	// a lot of the worst complexity in implementation.
	for (unsigned i = 0; i < 3; i++)
	{
		float iw = 1.0f / prim.vertices[i].w;
		prim.vertices[i].x *= iw;
		prim.vertices[i].y *= iw;
		prim.vertices[i].z *= iw;

		// Fixed point consideration.
		// Rescale inverse W for improved interpolation accuracy.
		// 1/w is now scaled to be maximum 0.5.
		iw *= min_w;
		prim.vertices[i].u = prim.vertices[i].u * iw;
		prim.vertices[i].v = prim.vertices[i].v * iw;
		prim.vertices[i].w = iw;

		// Color is intentionally not perspective correct.

		// Apply viewport transform for X/Y.
		prim.vertices[i].x = vp.x + (0.5f * prim.vertices[i].x + 0.5f) * vp.width;
		prim.vertices[i].y = vp.y + (0.5f * prim.vertices[i].y + 0.5f) * vp.height;
	}

	// After the viewport transform we can clip X/Y on guard bard rather than the strict [-w, w] clipping scheme
	// which we would normally have to do.

	// Clip -X on guard bard.
	unsigned count = clip_triangles(tmp_a, &prim, 1, 0, -1024.0f);
	// Clip +X on guard band.
	count = clip_triangles(tmp_b, tmp_a, count, 0, +1023.0f);
	// Clip -Y on guard band.
	count = clip_triangles(tmp_a, tmp_b, count, 1, -2048.0f);
	// Clip +Y on guard band.
	count = clip_triangles(tmp_b, tmp_a, count, 1, +2047.0f);

	// We could just support depth clamp, but it would make fixed point implementations very difficult ...
	// Clip near, before viewport transform.
	count = clip_triangles(tmp_a, tmp_b, count, 2, 0.0f);
	// Clip far, before viewport transform.
	count = clip_triangles(tmp_b, tmp_a, count, 2, +1.0f);

	unsigned output_count = 0;
	for (unsigned i = 0; i < count; i++)
	{
		auto &tmp_prim = tmp_b[i];
		for (unsigned j = 0; j < 3; j++)
		{
			// Apply viewport transform for Z after clipping.
			tmp_prim.vertices[j].z = vp.min_depth + tmp_prim.vertices[j].z * (vp.max_depth - vp.min_depth);
		}

		// Finally, we can perform triangle setup.
		if (setup_triangle(setup[output_count], tmp_b[i], mode))
			output_count++;
	}

	return output_count;
}

unsigned setup_clipped_triangles(PrimitiveSetup *setup, const InputPrimitive &prim, CullMode mode, const ViewportTransform &vp)
{
	// First, we need to clip if we have negative W coordinates.
	// Don't clip against 0, since we have no way to deal with infinities in the rasterizer later.
	// W of 1.0 / 1024.0 is super close to eye anyways.
	static const float MIN_W = 1.0f / 1024.0f;

	unsigned clip_code_w = get_clip_code_low(prim, MIN_W, 3);
	InputPrimitive clipped_w[2];
	unsigned clipped_w_count = clip_component(clipped_w, prim, 3, MIN_W, clip_code_w);
	unsigned output_count = 0;

	for (unsigned i = 0; i < clipped_w_count; i++)
	{
		unsigned count = setup_clipped_triangles_clipped_w(setup, clipped_w[i], mode, vp);
		setup += count;
		output_count += count;
	}
	return output_count;
}
}
