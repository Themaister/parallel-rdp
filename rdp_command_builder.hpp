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

#include "triangle_converter.hpp"
#include "rdp_dump.hpp"

namespace RDP
{

class CommandBuilder : public CommandInterface
{
public:
	void set_viewport(const ViewportTransform &viewport);
	unsigned draw_triangle(const InputPrimitive &prim);
	void fill_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
	void fill_rectangle_subpixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
	void set_scissor(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool interlace = false, bool keepodd = false);
	void set_scissor_subpixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool interlace = false, bool keepodd = false);
	void tex_rect(unsigned tile, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t s, uint16_t t, uint16_t dsdx, uint16_t dtdy);
	void tex_rect_flip(unsigned tile, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t s, uint16_t t, uint16_t dsdx, uint16_t dtdy);

	size_t get_rdram_size() const override;
	size_t get_hidden_rdram_size() const override;

	void set_cycle_type(CycleType type);

	void set_combiner_1cycle(const CombinerInputs &inputs);
	void set_combiner_2cycle(const CombinerInputs &first, const CombinerInputs &second);

	void set_blend_mode(unsigned cycle,
	                    BlendMode1A blend_1a, BlendMode1B blend_1b,
	                    BlendMode2A blend_2a, BlendMode2B blend_2b);

	void end_frame();

	void set_color_image(TextureFormat fmt, TextureSize size, uint32_t addr, uint32_t width);
	void set_depth_image(uint32_t addr);

	void set_texture_image(uint32_t addr, TextureFormat fmt, TextureSize size, uint32_t width);
	void set_tile(uint32_t tile, const TileMeta &info);
	void set_tile_size(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height);
	void set_tile_size_subpixels(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height);
	void load_tile(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height);
	void load_tile_subpixels(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height);
	void load_block(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned dt);
	void load_tlut(uint32_t tile, unsigned x, unsigned y, unsigned width, unsigned height);
	void set_tlut(bool enable, bool ia_type);
	void set_tex_lod_enable(bool enable);
	void set_tex_lod_sharpen_enable(bool enable);
	void set_tex_lod_detail_enable(bool enable);
	void set_image_read_enable(bool enable);
	void set_color_on_coverage(bool enable);
	void set_command_interface(CommandListenerInterface *iface) override;

	void set_enable_blend(bool enable);
	void set_enable_aa(bool enable);
	void set_depth_test(bool enable);
	void set_depth_write(bool enable);
	void set_perspective(bool enable);
	void set_dither(RGBDitherMode mode);
	void set_dither(AlphaDitherMode mode);
	void set_alpha_test(bool enable);
	void set_alpha_test_dither(bool enable);
	void set_cvg_times_alpha(bool enable);
	void set_alpha_cvg_select(bool enable);
	void set_z_mode(ZMode mode);
	void set_coverage_mode(CoverageMode mode);
	void set_enable_sample_quad(bool enable);
	void set_enable_mid_texel(bool enable);
	void set_enable_convert_one(bool enable);

	void set_env_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void set_blend_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void set_fog_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void set_primitive_color(uint8_t min_lod, uint8_t prim_lod_frac, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void set_fill_color(uint32_t col);
	void set_convert(uint16_t k0, uint16_t k1, uint16_t k2, uint16_t k3, uint16_t k4, uint16_t k5);

	void set_primitive_depth(uint16_t prim_depth, uint16_t prim_dz);
	void set_enable_primitive_depth(bool enable);

private:
	CommandListenerInterface *iface = nullptr;

	ViewportTransform viewport = {};

	void submit_clipped_primitive(const PrimitiveSetup &setup);
	void flush_default_state();

	struct
	{
		RGBDitherMode rgb_dither = RGBDitherMode::Off;
		AlphaDitherMode alpha_dither = AlphaDitherMode::Off;
		CycleType cycle_type = CycleType::Cycle1;
		ZMode z_mode = ZMode::Opaque;
		CoverageMode coverage_mode = CoverageMode::Clamp;
		bool aa = false;
		bool depth_test = false;
		bool alpha_test_dither = false;
		bool depth_write = false;
		bool perspective = false;
		bool alpha_test = false;
		bool tlut = false;
		bool tlut_ia_type = false;
		bool cvg_times_alpha = false;
		bool alpha_cvg_select = false;
		bool tex_lod_enable = false;
		bool tex_lod_sharpen_enable = false;
		bool tex_lod_detail_enable = false;
		bool image_read_enable = false;
		bool color_on_coverage = false;
		bool primitive_depth = false;
		bool sample_quad = false;
		bool mid_texel = false;
		bool convert_one = false;

		BlendModes blender_cycles[2] = {
			{ BlendMode1A::PixelColor, BlendMode1B::PixelAlpha, BlendMode2A::PixelColor, BlendMode2B::InvPixelAlpha },
			{ BlendMode1A::PixelColor, BlendMode1B::PixelAlpha, BlendMode2A::PixelColor, BlendMode2B::InvPixelAlpha },
		};
		bool blend_en = false;
	} other_modes;
};
}