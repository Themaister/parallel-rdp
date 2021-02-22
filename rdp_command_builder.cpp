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

#include "rdp_command_builder.hpp"
#include "rdp_dump.hpp"
#include "logging.hpp"
#include <assert.h>

namespace RDP
{
void CommandBuilder::set_command_interface(CommandListenerInterface *iface_)
{
	iface = iface_;
}

void CommandBuilder::set_viewport(const ViewportTransform &viewport_)
{
	viewport = viewport_;
}

unsigned CommandBuilder::draw_triangle(const InputPrimitive &prim)
{
	PrimitiveSetup prims[8];
	unsigned count = setup_clipped_triangles(prims, prim, CullMode::None, viewport);
	for (unsigned i = 0; i < count; i++)
		submit_clipped_primitive(prims[i]);
	return count;
}

unsigned CommandBuilder::draw_triangle_ym_out_of_range(const InputPrimitive &prim, int ym_delta)
{
	PrimitiveSetup prims[8];
	unsigned count = setup_clipped_triangles(prims, prim, CullMode::None, viewport);
	for (unsigned i = 0; i < count; i++)
	{
		if (ym_delta < 0)
			prims[i].pos.y_mid = prims[i].pos.y_lo + ym_delta;
		else if (ym_delta > 0)
			prims[i].pos.y_hi = prims[i].pos.y_hi + ym_delta;
		submit_clipped_primitive(prims[i]);
	}
	return count;
}

void CommandBuilder::end_frame()
{
	// Set some known working register state from a dump. No idea what all the bits do yet.
	uint32_t words[2] = {};
	words[0] = uint32_t(Op::SyncFull) << 24;
	iface->command(Op::SyncFull, 2, words);

	iface->set_vi_register(VIRegister::Control,
	                       VI_CONTROL_TYPE_RGBA5551_BIT |
	                       VI_CONTROL_AA_MODE_RESAMP_EXTRA_BIT |
	                       VI_CONTROL_DIVOT_ENABLE_BIT |
	                       VI_CONTROL_GAMMA_ENABLE_BIT |
	                       VI_CONTROL_DITHER_FILTER_ENABLE_BIT);
	iface->set_vi_register(VIRegister::Origin, 64);
	iface->set_vi_register(VIRegister::Width, 320);
	iface->set_vi_register(VIRegister::VCurrentLine, 0);
	iface->set_vi_register(VIRegister::VSync, VI_V_SYNC_NTSC);
	iface->set_vi_register(VIRegister::HStart, make_vi_start_register(VI_H_OFFSET_NTSC, VI_H_OFFSET_NTSC + 640));
	iface->set_vi_register(VIRegister::VStart, make_vi_start_register(VI_V_OFFSET_NTSC, VI_V_OFFSET_NTSC + 224));
	iface->set_vi_register(VIRegister::XScale, make_vi_scale_register(512, 1345));
	iface->set_vi_register(VIRegister::YScale, make_vi_scale_register(1024, 1345));
	iface->signal_complete();
	iface->end_frame();
}

void CommandBuilder::set_enable_aa(bool enable)
{
	other_modes.aa = enable;
}

void CommandBuilder::set_depth_test(bool enable)
{
	other_modes.depth_test = enable;
}

void CommandBuilder::set_depth_write(bool enable)
{
	other_modes.depth_write = enable;
}

void CommandBuilder::set_perspective(bool enable)
{
	other_modes.perspective = enable;
}

void CommandBuilder::set_dither(RGBDitherMode mode)
{
	other_modes.rgb_dither = mode;
}

void CommandBuilder::set_dither(AlphaDitherMode mode)
{
	other_modes.alpha_dither = mode;
}

void CommandBuilder::set_alpha_test(bool enable)
{
	other_modes.alpha_test = enable;
}

void CommandBuilder::set_alpha_test_dither(bool enable)
{
	other_modes.alpha_test_dither = enable;
}

void CommandBuilder::set_cvg_times_alpha(bool enable)
{
	other_modes.cvg_times_alpha = enable;
}

void CommandBuilder::set_alpha_cvg_select(bool enable)
{
	other_modes.alpha_cvg_select = enable;
}

void CommandBuilder::set_z_mode(ZMode mode)
{
	other_modes.z_mode = mode;
}

void CommandBuilder::set_coverage_mode(CoverageMode mode)
{
	other_modes.coverage_mode = mode;
}

void CommandBuilder::set_enable_sample_quad(bool enable)
{
	other_modes.sample_quad = enable;
}

void CommandBuilder::set_blend_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetBlendColor) << 24;
	cmd[1] |= uint32_t(r) << 24;
	cmd[1] |= uint32_t(g) << 16;
	cmd[1] |= uint32_t(b) << 8;
	cmd[1] |= uint32_t(a) << 0;

	iface->command(Op::SetBlendColor, 2, cmd);
}

void CommandBuilder::set_env_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetEnvColor) << 24;
	cmd[1] |= uint32_t(r) << 24;
	cmd[1] |= uint32_t(g) << 16;
	cmd[1] |= uint32_t(b) << 8;
	cmd[1] |= uint32_t(a) << 0;

	iface->command(Op::SetEnvColor, 2, cmd);
}

void CommandBuilder::set_fog_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetFogColor) << 24;
	cmd[1] |= uint32_t(r) << 24;
	cmd[1] |= uint32_t(g) << 16;
	cmd[1] |= uint32_t(b) << 8;
	cmd[1] |= uint32_t(a) << 0;

	iface->command(Op::SetFogColor, 2, cmd);
}

void CommandBuilder::set_primitive_color(uint8_t min_lod, uint8_t prim_lod_frac, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetPrimColor) << 24;
	cmd[0] |= min_lod << 8;
	cmd[0] |= prim_lod_frac;
	cmd[1] |= uint32_t(r) << 24;
	cmd[1] |= uint32_t(g) << 16;
	cmd[1] |= uint32_t(b) << 8;
	cmd[1] |= uint32_t(a) << 0;

	iface->command(Op::SetPrimColor, 2, cmd);
}

void CommandBuilder::set_fill_color(uint32_t col)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetFillColor) << 24;
	cmd[1] = col;
	iface->command(Op::SetFillColor, 2, cmd);
}

void CommandBuilder::set_convert(uint16_t k0, uint16_t k1, uint16_t k2, uint16_t k3, uint16_t k4, uint16_t k5)
{
	uint32_t cmd[2] = {};
	cmd[0] = uint32_t(Op::SetConvert) << 24;

	uint64_t word = 0;
	word |= uint64_t(k5 & 0x1ff) << 0;
	word |= uint64_t(k4 & 0x1ff) << 9;
	word |= uint64_t(k3 & 0x1ff) << 18;
	word |= uint64_t(k2 & 0x1ff) << 27;
	word |= uint64_t(k1 & 0x1ff) << 36;
	word |= uint64_t(k0 & 0x1ff) << 45;

	cmd[0] |= word >> 32;
	cmd[1] = uint32_t(word);
	iface->command(Op::SetConvert, 2, cmd);
}

void CommandBuilder::set_key_r(uint32_t width, uint32_t center, uint32_t scale)
{
	uint32_t cmd[2] = {};
	cmd[0] = uint32_t(Op::SetKeyR) << 24;

	cmd[1] |= (width & 0xfff) << 16;
	cmd[1] |= (center & 0xff) << 8;
	cmd[1] |= (scale & 0xff) << 0;

	iface->command(Op::SetKeyR, 2, cmd);
}

void CommandBuilder::set_key_gb(uint32_t g_width, uint32_t g_center, uint32_t g_scale,
                                uint32_t b_width, uint32_t b_center, uint32_t b_scale)
{
	uint32_t cmd[2] = {};
	cmd[0] = uint32_t(Op::SetKeyGB) << 24;

	cmd[0] |= (g_width & 0xfff) << 12;
	cmd[0] |= (b_width & 0xfff) << 0;

	cmd[1] |= (g_center & 0xff) << 24;
	cmd[1] |= (g_scale & 0xff) << 16;
	cmd[1] |= (b_center & 0xff) << 8;
	cmd[1] |= (b_scale & 0xff) << 0;

	iface->command(Op::SetKeyGB, 2, cmd);
}

void CommandBuilder::set_primitive_depth(uint16_t prim_depth, uint16_t prim_dz)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetPrimDepth) << 24;
	cmd[1] = (uint32_t(prim_depth) << 16) | prim_dz;
	iface->command(Op::SetPrimDepth, 2, cmd);
}

void CommandBuilder::set_enable_primitive_depth(bool enable)
{
	other_modes.primitive_depth = enable;
}

template <typename T>
static inline uint32_t mask(T val, unsigned bits)
{
	return uint32_t(val & ((1 << bits) - 1));
}

void CommandBuilder::flush_default_state()
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetOtherModes) << 24;
	cmd[0] |= uint32_t(other_modes.cycle_type) << 20;
	if (other_modes.perspective)
		cmd[0] |= 1 << 19;
	if (other_modes.tex_lod_detail_enable)
		cmd[0] |= 1 << 18;
	if (other_modes.tex_lod_sharpen_enable)
		cmd[0] |= 1 << 17;
	if (other_modes.tex_lod_enable)
		cmd[0] |= 1 << 16;
	if (other_modes.tlut)
		cmd[0] |= 1 << 15;
	if (other_modes.tlut_ia_type)
		cmd[0] |= 1 << 14;
	if (other_modes.sample_quad)
		cmd[0] |= 1 << 13;
	if (other_modes.mid_texel)
		cmd[0] |= 1 << 12;
	if (other_modes.bilerps[0])
		cmd[0] |= 1 << 11;
	if (other_modes.bilerps[1])
		cmd[0] |= 1 << 10;
	if (other_modes.convert_one)
		cmd[0] |= 1 << 9;
	cmd[0] |= uint32_t(other_modes.rgb_dither) << 6;
	cmd[0] |= uint32_t(other_modes.alpha_dither) << 4;
	cmd[1] |= uint32_t(other_modes.blender_cycles[0].blend_1a) << 30;
	cmd[1] |= uint32_t(other_modes.blender_cycles[1].blend_1a) << 28;
	cmd[1] |= uint32_t(other_modes.blender_cycles[0].blend_1b) << 26;
	cmd[1] |= uint32_t(other_modes.blender_cycles[1].blend_1b) << 24;
	cmd[1] |= uint32_t(other_modes.blender_cycles[0].blend_2a) << 22;
	cmd[1] |= uint32_t(other_modes.blender_cycles[1].blend_2a) << 20;
	cmd[1] |= uint32_t(other_modes.blender_cycles[0].blend_2b) << 18;
	cmd[1] |= uint32_t(other_modes.blender_cycles[1].blend_2b) << 16;
	if (other_modes.blend_en)
		cmd[1] |= 1 << 14;
	if (other_modes.alpha_cvg_select)
		cmd[1] |= 1 << 13;
	if (other_modes.cvg_times_alpha)
		cmd[1] |= 1 << 12;
	cmd[1] |= uint32_t(other_modes.z_mode) << 10;
	cmd[1] |= uint32_t(other_modes.coverage_mode) << 8;
	if (other_modes.color_on_coverage)
		cmd[1] |= 1 << 7;
	if (other_modes.image_read_enable)
		cmd[1] |= 1 << 6;
	if (other_modes.depth_write)
		cmd[1] |= 1 << 5;
	if (other_modes.depth_test)
		cmd[1] |= 1 << 4;
	if (other_modes.aa)
		cmd[1] |= 1 << 3;
	if (other_modes.primitive_depth)
		cmd[1] |= 1 << 2;
	if (other_modes.alpha_test_dither)
		cmd[1] |= 1 << 1;
	if (other_modes.alpha_test)
		cmd[1] |= 1 << 0;
	iface->command(Op::SetOtherModes, 2, cmd);
}

void CommandBuilder::set_color_image(TextureFormat fmt, TextureSize size, uint32_t addr, uint32_t width)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetColorImage) << 24;
	cmd[0] |= uint32_t(fmt) << 21;
	cmd[0] |= uint32_t(size) << 19;
	cmd[0] |= (width - 1) & 1023;
	cmd[1] = addr;
	iface->command(Op::SetColorImage, 2, cmd);
}

void CommandBuilder::set_depth_image(uint32_t addr)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetMaskImage) << 24;
	cmd[1] = addr;
	iface->command(Op::SetMaskImage, 2, cmd);
}

void CommandBuilder::submit_clipped_primitive(const PrimitiveSetup &setup)
{
	flush_default_state();

	uint32_t cmd[44] = {};
	cmd[0] |= uint32_t(Op::ShadeTextureZBufferTriangle) << 24;
	if (!(setup.pos.flags & PRIMITIVE_RIGHT_MAJOR_BIT))
		cmd[0] |= 1u << 23;

	constexpr unsigned tile = 0;
	constexpr unsigned max_level = 6;
	cmd[0] |= tile << 16;
	cmd[0] |= max_level << 19;

	cmd[1] |= mask(setup.pos.y_lo, 14);
	cmd[0] |= mask(setup.pos.y_hi, 14);
	cmd[1] |= mask(setup.pos.y_mid, 14) << 16;

	cmd[2] = mask(setup.pos.x_c, 28);
	cmd[3] = mask(setup.pos.dxdy_c, 30);
	cmd[4] = mask(setup.pos.x_a, 28);

	// Need sign bit to deal with attribute interpolation.
	cmd[5] = setup.pos.dxdy_a;

	cmd[6] = mask(setup.pos.x_b, 28);
	cmd[7] = mask(setup.pos.dxdy_b, 30);

	// R
	cmd[8] |= setup.attr.c[0] & 0xffff0000;
	cmd[12] |= (setup.attr.c[0] << 16) & 0xffff0000;
	// G
	cmd[8] |= (setup.attr.c[1] >> 16) & 0xffff;
	cmd[12] |= setup.attr.c[1] & 0xffff;
	// B
	cmd[9] |= setup.attr.c[2] & 0xffff0000;
	cmd[13] |= (setup.attr.c[2] << 16) & 0xffff0000;
	// A
	cmd[9] |= (setup.attr.c[3] >> 16) & 0xffff;
	cmd[13] |= setup.attr.c[3] & 0xffff;

	// dRdx
	cmd[10] |= setup.attr.dcdx[0] & 0xffff0000;
	cmd[14] |= (setup.attr.dcdx[0] << 16) & 0xffff0000;
	// dGdx
	cmd[10] |= (setup.attr.dcdx[1] >> 16) & 0xffff;
	cmd[14] |= setup.attr.dcdx[1] & 0xffff;
	// dBdx
	cmd[11] |= setup.attr.dcdx[2] & 0xffff0000;
	cmd[15] |= (setup.attr.dcdx[2] << 16) & 0xffff0000;
	// dAdx
	cmd[11] |= (setup.attr.dcdx[3] >> 16) & 0xffff;
	cmd[15] |= setup.attr.dcdx[3] & 0xffff;

	// dRde
	cmd[16] |= setup.attr.dcde[0] & 0xffff0000;
	cmd[20] |= (setup.attr.dcde[0] << 16) & 0xffff0000;
	// dGde
	cmd[16] |= (setup.attr.dcde[1] >> 16) & 0xffff;
	cmd[20] |= setup.attr.dcde[1] & 0xffff;
	// dBde
	cmd[17] |= setup.attr.dcde[2] & 0xffff0000;
	cmd[21] |= (setup.attr.dcde[2] << 16) & 0xffff0000;
	// dAde
	cmd[17] |= (setup.attr.dcde[3] >> 16) & 0xffff;
	cmd[21] |= setup.attr.dcde[3] & 0xffff;

	// dRdy
	cmd[18] |= setup.attr.dcdy[0] & 0xffff0000;
	cmd[22] |= (setup.attr.dcdy[0] << 16) & 0xffff0000;
	// dGdy
	cmd[18] |= (setup.attr.dcdy[1] >> 16) & 0xffff;
	cmd[22] |= setup.attr.dcdy[1] & 0xffff;
	// dBdy
	cmd[19] |= setup.attr.dcdy[2] & 0xffff0000;
	cmd[23] |= (setup.attr.dcdy[2] << 16) & 0xffff0000;
	// dAdy
	cmd[19] |= (setup.attr.dcdy[3] >> 16) & 0xffff;
	cmd[23] |= setup.attr.dcdy[3] & 0xffff;

	// S
	cmd[24] |= setup.attr.u & 0xffff0000;
	cmd[28] |= (setup.attr.u << 16) & 0xffff0000;
	// T
	cmd[24] |= (setup.attr.v >> 16) & 0xffff;
	cmd[28] |= setup.attr.v & 0xffff;
	// W
	cmd[25] |= setup.attr.w & 0xffff0000;
	cmd[29] |= (setup.attr.w << 16) & 0xffff0000;
	// dSdx
	cmd[26] |= setup.attr.dudx & 0xffff0000;
	cmd[30] |= (setup.attr.dudx << 16) & 0xffff0000;
	// dTdx
	cmd[26] |= (setup.attr.dvdx >> 16) & 0xffff;
	cmd[30] |= setup.attr.dvdx & 0xffff;
	// dWdx
	cmd[27] |= setup.attr.dwdx & 0xffff0000;
	cmd[31] |= (setup.attr.dwdx << 16) & 0xffff0000;
	// dSde
	cmd[32] |= setup.attr.dude & 0xffff0000;
	cmd[36] |= (setup.attr.dude << 16) & 0xffff0000;
	// dTde
	cmd[32] |= (setup.attr.dvde >> 16) & 0xffff;
	cmd[36] |= setup.attr.dvde & 0xffff;
	// dWde
	cmd[33] |= setup.attr.dwde & 0xffff0000;
	cmd[37] |= (setup.attr.dwde << 16) & 0xffff0000;
	// dSdy
	cmd[34] |= setup.attr.dudy & 0xffff0000;
	cmd[38] |= (setup.attr.dudy << 16) & 0xffff0000;
	// dTdy
	cmd[34] |= (setup.attr.dvdy >> 16) & 0xffff;
	cmd[38] |= setup.attr.dvdy & 0xffff;
	// dWdy
	cmd[35] |= setup.attr.dwdy & 0xffff0000;
	cmd[39] |= (setup.attr.dwdy << 16) & 0xffff0000;

	cmd[40] = setup.attr.z;
	cmd[41] = setup.attr.dzdx;
	cmd[42] = setup.attr.dzde;
	cmd[43] = setup.attr.dzdy;

	iface->command(Op::ShadeTextureZBufferTriangle, 44, cmd);
}

void CommandBuilder::set_cycle_type(CycleType type)
{
	other_modes.cycle_type = type;
}

void CommandBuilder::set_enable_blend(bool enable)
{
	other_modes.blend_en = enable;
}

void CommandBuilder::set_blend_mode(unsigned cycle,
                                    BlendMode1A blend_1a, BlendMode1B blend_1b,
                                    BlendMode2A blend_2a, BlendMode2B blend_2b)
{
	auto &blend = other_modes.blender_cycles[cycle];
	blend.blend_1a = blend_1a;
	blend.blend_1b = blend_1b;
	blend.blend_2a = blend_2a;
	blend.blend_2b = blend_2b;
}

void CommandBuilder::set_combiner_1cycle(const CombinerInputs &inputs)
{
	set_combiner_2cycle(inputs, inputs);
}

void CommandBuilder::set_combiner_2cycle(const CombinerInputs &first, const CombinerInputs &second)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetCombine) << 24;

	cmd[0] |= uint32_t(first.rgb.muladd) << 20;
	cmd[0] |= uint32_t(first.rgb.mul) << 15;
	cmd[0] |= uint32_t(first.alpha.muladd) << 12;
	cmd[0] |= uint32_t(first.alpha.mul) << 9;
	cmd[0] |= uint32_t(second.rgb.muladd) << 5;
	cmd[0] |= uint32_t(second.rgb.mul) << 0;

	cmd[1] |= uint32_t(first.rgb.mulsub) << 28;
	cmd[1] |= uint32_t(second.rgb.mulsub) << 24;
	cmd[1] |= uint32_t(second.alpha.muladd) << 21;
	cmd[1] |= uint32_t(second.alpha.mul) << 18;
	cmd[1] |= uint32_t(first.rgb.add) << 15;
	cmd[1] |= uint32_t(first.alpha.mulsub) << 12;
	cmd[1] |= uint32_t(first.alpha.add) << 9;
	cmd[1] |= uint32_t(second.rgb.add) << 6;
	cmd[1] |= uint32_t(second.alpha.mulsub) << 3;
	cmd[1] |= uint32_t(second.alpha.add) << 0;

	iface->command(Op::SetCombine, 2, cmd);
}

void CommandBuilder::fill_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	flush_default_state();
	fill_rectangle_subpixels(x << 2, y << 2, width << 2, height << 2);
}

void CommandBuilder::fill_rectangle_subpixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::FillRectangle) << 24;

	cmd[0] |= (((x + width - 4) & 0xfff) << 12);
	cmd[1] |= ((x & 0xfff) << 12);
	cmd[0] |= (((y + height - 4) & 0xfff) << 0);
	cmd[1] |= ((y & 0xfff) << 0);

	iface->command(Op::FillRectangle, 2, cmd);
}

void CommandBuilder::tex_rect(unsigned tile, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                              uint16_t s, uint16_t t, uint16_t dsdx, uint16_t dtdy)
{
	flush_default_state();

	uint32_t cmd[4] = {};
	cmd[0] |= uint32_t(Op::TextureRectangle) << 24;

	cmd[0] |= (((x + width - 4) & 0xfff) << 12);
	cmd[1] |= ((x & 0xfff) << 12);
	cmd[0] |= (((y + height - 4) & 0xfff) << 0);
	cmd[1] |= ((y & 0xfff) << 0);

	cmd[1] |= (tile & 7) << 24;

	cmd[2] |= uint32_t(s) << 16;
	cmd[2] |= uint32_t(t);
	cmd[3] |= uint32_t(dsdx) << 16;
	cmd[3] |= uint32_t(dtdy);

	iface->command(Op::TextureRectangle, 4, cmd);
}

void CommandBuilder::tex_rect_flip(unsigned tile, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                                   uint16_t s, uint16_t t, uint16_t dsdx, uint16_t dtdy)
{
	flush_default_state();

	uint32_t cmd[4] = {};
	cmd[0] |= uint32_t(Op::TextureRectangleFlip) << 24;

	cmd[0] |= (((x + width - 4) & 0xfff) << 12);
	cmd[1] |= ((x & 0xfff) << 12);
	cmd[0] |= (((y + height - 4) & 0xfff) << 0);
	cmd[1] |= ((y & 0xfff) << 0);

	cmd[1] |= (tile & 7) << 24;

	cmd[2] |= uint32_t(s) << 16;
	cmd[2] |= uint32_t(t);
	cmd[3] |= uint32_t(dsdx) << 16;
	cmd[3] |= uint32_t(dtdy);

	iface->command(Op::TextureRectangleFlip, 4, cmd);
}

void CommandBuilder::set_scissor_subpixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool interlace, bool keepodd)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetScissor) << 24;

	unsigned xh = x, yh = y, xl = (x + width), yl = (y + height);
	assert(xh < 0x1000);
	assert(yh < 0x1000);
	assert(xl < 0x1000);
	assert(yl < 0x1000);

	cmd[0] |= xh << 12;
	cmd[0] |= yh << 0;
	cmd[1] |= xl << 12;
	cmd[1] |= yl << 0;

	if (interlace)
		cmd[1] |= 1 << 25;
	if (keepodd)
		cmd[1] |= 1 << 24;

	iface->command(Op::SetScissor, 2, cmd);
}

void CommandBuilder::set_scissor(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool interlace, bool keepodd)
{
	set_scissor_subpixels(x << 2, y << 2, width << 2, height << 2, interlace, keepodd);
}

void CommandBuilder::set_texture_image(uint32_t addr, TextureFormat fmt, TextureSize size, uint32_t width)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetTextureImage) << 24;

	cmd[0] |= uint32_t(fmt) << 21;
	cmd[0] |= uint32_t(size) << 19;
	cmd[0] |= (width - 1) & 0x3ff;
	cmd[1] |= addr & 0x00ffffffu;

	iface->command(Op::SetTextureImage, 2, cmd);
}

void CommandBuilder::set_tile(uint32_t tile, const TileMeta &info)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetTile) << 24;

	cmd[1] |= info.shift_s << 0;
	cmd[1] |= info.mask_s << 4;
	cmd[1] |= uint32_t(!!(info.flags & TILE_INFO_MIRROR_S_BIT)) << 8;
	cmd[1] |= uint32_t(!!(info.flags & TILE_INFO_CLAMP_S_BIT)) << 9;

	cmd[1] |= info.shift_t << 10;
	cmd[1] |= info.mask_t << 14;
	cmd[1] |= uint32_t(!!(info.flags & TILE_INFO_MIRROR_T_BIT)) << 18;
	cmd[1] |= uint32_t(!!(info.flags & TILE_INFO_CLAMP_T_BIT)) << 19;

	cmd[1] |= info.palette << 20;
	cmd[1] |= tile << 24;

	assert((info.offset & 7) == 0);
	assert((info.stride & 7) == 0);
	cmd[0] |= (info.offset >> 3) << 0;
	cmd[0] |= (info.stride >> 3) << 9;
	cmd[0] |= uint32_t(info.size) << 19;
	cmd[0] |= uint32_t(info.fmt) << 21;

	iface->command(Op::SetTile, 2, cmd);
}

void CommandBuilder::load_tile_subpixels(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::LoadTile) << 24;
	cmd[1] |= tile << 24;

	unsigned sl = x & 0xfff;
	unsigned tl = y & 0xfff;
	unsigned sh = (x + width - 4) & 0xfff;
	unsigned th = (y + height - 4) & 0xfff;

	cmd[0] |= sl << 12;
	cmd[0] |= tl << 0;
	cmd[1] |= sh << 12;
	cmd[1] |= th << 0;
	iface->command(Op::LoadTile, 2, cmd);
}

void CommandBuilder::set_tile_size_subpixels(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::SetTileSize) << 24;
	cmd[1] |= tile << 24;

	unsigned sl = x & 0xfff;
	unsigned tl = y & 0xfff;
	unsigned sh = (x + width - 4) & 0xfff;
	unsigned th = (y + height - 4) & 0xfff;

	cmd[0] |= sl << 12;
	cmd[0] |= tl << 0;
	cmd[1] |= sh << 12;
	cmd[1] |= th << 0;
	iface->command(Op::SetTileSize, 2, cmd);
}

void CommandBuilder::set_tile_size(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height)
{
	set_tile_size_subpixels(tile, x << 2, y << 2, width << 2, height << 2);
}

void CommandBuilder::load_tlut(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::LoadTLut) << 24;
	cmd[1] |= tile << 24;

	unsigned sl = (x << 2) & 0xfff;
	unsigned tl = (y << 2) & 0xfff;
	unsigned sh = ((x + width - 1) << 2) & 0xfff;
	unsigned th = ((y + height - 1) << 2) & 0xfff;

	cmd[0] |= sl << 12;
	cmd[0] |= tl << 0;
	cmd[1] |= sh << 12;
	cmd[1] |= th << 0;
	iface->command(Op::LoadTLut, 2, cmd);
}

void CommandBuilder::load_block(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned dt)
{
	uint32_t cmd[2] = {};
	cmd[0] |= uint32_t(Op::LoadBlock) << 24;
	cmd[1] |= tile << 24;

	unsigned sl = x & 0xfff;
	unsigned tl = y & 0xfff;
	unsigned sh = (x + width - 1) & 0xfff;
	unsigned th = dt & 0xfff;

	cmd[0] |= sl << 12;
	cmd[0] |= tl << 0;
	cmd[1] |= sh << 12;
	cmd[1] |= th << 0;
	iface->command(Op::LoadBlock, 2, cmd);
}

void CommandBuilder::load_tile(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height)
{
	load_tile_subpixels(tile, x << 2, y << 2, width << 2, height << 2);
}

size_t CommandBuilder::get_rdram_size() const
{
	return 4 * 1024 * 1024;
}

size_t CommandBuilder::get_hidden_rdram_size() const
{
	return 2 * 1024 * 1024;
}

void CommandBuilder::set_tlut(bool enable, bool ia_type)
{
	other_modes.tlut = enable;
	other_modes.tlut_ia_type = ia_type;
}

void CommandBuilder::set_tex_lod_enable(bool enable)
{
	other_modes.tex_lod_enable = enable;
}

void CommandBuilder::set_tex_lod_sharpen_enable(bool enable)
{
	other_modes.tex_lod_sharpen_enable = enable;
}

void CommandBuilder::set_tex_lod_detail_enable(bool enable)
{
	other_modes.tex_lod_detail_enable = enable;
}

void CommandBuilder::set_image_read_enable(bool enable)
{
	other_modes.image_read_enable = enable;
}

void CommandBuilder::set_color_on_coverage(bool enable)
{
	other_modes.color_on_coverage = enable;
}

void CommandBuilder::set_enable_mid_texel(bool enable)
{
	other_modes.mid_texel = enable;
}

void CommandBuilder::set_enable_convert_one(bool enable)
{
	other_modes.convert_one = enable;
}

void CommandBuilder::set_enable_bilerp_cycle(unsigned cycle, bool enable)
{
	assert(cycle < 2);
	other_modes.bilerps[cycle] = enable;
}
}