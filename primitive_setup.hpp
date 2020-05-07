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

#pragma once

#include <stdint.h>

namespace RDP
{
enum { SUBPIXELS_LOG2 = 2 };

enum PrimitiveFlagBits
{
	PRIMITIVE_RIGHT_MAJOR_BIT = 1 << 0,
	PRIMITIVE_PERSPECTIVE_CORRECT_BIT = 1 << 1,
	PRIMITIVE_FLAG_MAX_ENUM = 0x7fff
};

using PrimitiveFlags = uint16_t;

struct PrimitiveSetupPos
{
	int32_t x_a, x_b, x_c;
	int32_t dxdy_a, dxdy_b, dxdy_c;
	int16_t y_lo, y_mid, y_hi;
	uint16_t flags;
};

struct PrimitiveSetupAttr
{
	int32_t c[4];
	int32_t dcdx[4];
	int32_t dcde[4];
	int32_t dcdy[4];

	int32_t z, dzdx, dzde, dzdy;
	int32_t u, v, w;
	int32_t dudx, dvdx, dwdx;
	int32_t dude, dvde, dwde;
	int32_t dudy, dvdy, dwdy;
};

struct PrimitiveSetup
{
	PrimitiveSetupPos pos;
	PrimitiveSetupAttr attr;
};

static_assert((sizeof(PrimitiveSetup) & 15) == 0, "PrimitiveSetup is not aligned to 16 bytes.");
}
