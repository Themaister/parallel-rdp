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

#include "global_managers_init.hpp"
#include "conformance_utils.hpp"
#include "rdp_dump.hpp"
#include "rdp_command_builder.hpp"
#include "cli_parser.hpp"
#include "context.hpp"
#include "device.hpp"

using namespace RDP;

static void generate_random_tmem(RNG &rng, ReplayerDriver &reference, ReplayerDriver &gpu)
{
	uint32_t random_data[1024];
	for (auto &r : random_data)
		r = uint32_t(rng.rnd());
	memcpy(reference.get_tmem(), random_data, sizeof(random_data));
	memcpy(gpu.get_tmem(), random_data, sizeof(random_data));
}

static void generate_random_input_primitive(RNG &rng, InputPrimitive &prim,
                                            bool generate_colors, bool generate_z, bool force_flip)
{
	prim = {};
	for (unsigned i = 0; i < 3; i++)
	{
		if (force_flip)
		{
			switch (i)
			{
			case 0:
				prim.vertices[i].x = rng.generate(-1.0f, -0.5f);
				prim.vertices[i].y = rng.generate(-1.0f, -0.5f);
				break;

			case 1:
				prim.vertices[i].x = rng.generate(0.5f, 1.0f);
				prim.vertices[i].y = rng.generate(-0.3f, +0.3f);
				break;

			default:
				prim.vertices[i].x = rng.generate(-1.0f, -0.5f);
				prim.vertices[i].y = rng.generate(0.5f, 1.0f);
				break;
			}
		}
		else
		{
			prim.vertices[i].x = rng.generate(-1.0f, 1.0f);
			prim.vertices[i].y = rng.generate(-1.0f, 1.0f);
		}

		prim.vertices[i].z = rng.generate(0.0f, 0.95f);
		prim.vertices[i].w = rng.generate(1.0f, 5.0f);

		prim.vertices[i].x *= prim.vertices[i].w;
		prim.vertices[i].y *= prim.vertices[i].w;
		prim.vertices[i].z *= prim.vertices[i].w;

		prim.vertices[i].u = rng.generate(-1000.0f, 1000.0f);
		prim.vertices[i].v = rng.generate(-1000.0f, 1000.0f);

		if (generate_colors)
		{
			for (unsigned j = 0; j < 4; j++)
				prim.vertices[i].color[j] = rng.generate(-5.0f, 5.0f);
		}
		else
		{
			if (generate_z)
			{
				for (unsigned j = 0; j < 4; j++)
					prim.vertices[i].color[j] = 0.0f;
			}
			else
			{
				for (unsigned j = 0; j < 4; j++)
					prim.vertices[i].color[j] = 1.0f;
			}
		}
	}
}

struct Arguments
{
	std::string suite_glob;
	std::string suite;
	unsigned lo = 0;
	unsigned hi = 10;
	bool verbose = false;
	bool capture = false;
};

struct RasterizationTestVariant
{
	bool interlace;
	bool aa;
	bool color;
	bool depth;
	bool color_depth_alias;
	bool depth_compare;
	bool texture;
	bool pipelined_texel1;
	bool tlut;
	bool tlut_type;
	bool mid_texel;
	bool convert_one;
	bool bilerp0 = true;
	bool bilerp1 = true;
	RGBDitherMode dither = RGBDitherMode::Off;
	AlphaDitherMode alpha_dither = AlphaDitherMode::Off;
	TextureFormat texture_format = TextureFormat::RGBA;
	TextureSize texture_size = TextureSize::Bpp4;
	CycleType cycle_type = CycleType::Cycle1;
	ZMode z_mode = ZMode::Opaque;
	CoverageMode coverage_mode = CoverageMode::Clamp;
	TextureFormat fb_fmt = TextureFormat::RGBA;
	TextureSize fb_size = TextureSize::Bpp16;
	unsigned primitive_count = 1;

	bool sample_quad = true;
	bool perspective;
	bool alpha_test;
	bool alpha_test_dither;
	bool cvg_times_alpha;
	bool alpha_cvg_select;
	bool combiner_inputs;
	bool blending;
	bool lod_frac;
	bool lod_sharpen;
	bool lod_detail;
	bool randomize_rdram;
	bool image_read_enable;
	bool color_on_coverage;
	bool force_flip;
	bool fill_rect;
	bool tex_rect;
	bool prim_depth;
	bool ym_out_of_range;
};

static RGBMulAdd generate_random_input(RNG &rng, RGBMulAdd input)
{
	if (input == RGBMulAdd::Combined)
		return input;

	switch (rng.rnd() % 5)
	{
	case 0:
		return RGBMulAdd::Env;
	case 1:
		return RGBMulAdd::One;
	case 2:
		return RGBMulAdd::Primitive;
	case 3:
		return RGBMulAdd::Noise;
	default:
		return input;
	}
}

static RGBMulSub generate_random_input(RNG &rng, RGBMulSub input)
{
	switch (rng.rnd() % 5)
	{
	case 0:
		return RGBMulSub::Env;
	case 1:
		return RGBMulSub::ConvertK4;
	case 2:
		return RGBMulSub::KeyCenter;
	case 3:
		return RGBMulSub::Primitive;
	default:
		return input;
	}
}

static RGBMul generate_random_input(RNG &rng, RGBMul input)
{
	if (input == RGBMul::CombinedAlpha)
		return input;

	switch (rng.rnd() % 7)
	{
	case 0:
		return RGBMul::Primitive;
	case 1:
		return RGBMul::PrimitiveAlpha;
	case 2:
		return RGBMul::Env;
	case 3:
		return RGBMul::EnvAlpha;
	case 4:
		return RGBMul::ConvertK5;
	case 5:
		return RGBMul::KeyScale;
	default:
		return input;
	}
}

static RGBAdd generate_random_input(RNG &rng, RGBAdd input)
{
	if (input == RGBAdd::Combined)
		return input;

	switch (rng.rnd() % 4)
	{
#if 0
	case 0:
		return RGBAdd::Shade;
#endif
	case 1:
		return RGBAdd::Env;
	case 2:
		return RGBAdd::One;
	default:
		return input;
	}
}

static AlphaAddSub generate_random_input(RNG &rng, AlphaAddSub input)
{
	if (input == AlphaAddSub::CombinedAlpha)
		return input;

	switch (rng.rnd() % 5)
	{
#if 0
	case 0:
		return AlphaAddSub::ShadeAlpha;
#endif
	case 1:
		return AlphaAddSub::EnvAlpha;
	case 2:
		return AlphaAddSub::One;
	case 3:
		return AlphaAddSub::PrimitiveAlpha;
	default:
		return input;
	}
}

static AlphaMul generate_random_input(RNG &rng, AlphaMul input)
{
	switch (rng.rnd() % 4)
	{
#if 0
	case 0:
		return AlphaMul::ShadeAlpha;
#endif
	case 1:
		return AlphaMul::EnvAlpha;
	case 2:
		return AlphaMul::PrimitiveAlpha;
	default:
		return input;
	}
}

static bool run_conformance_rasterization(ReplayerState &state, const Arguments &args, const RasterizationTestVariant &variant)
{
	InputPrimitive prim = {};
	RNG rng;

	state.builder.set_color_image(variant.fb_fmt, variant.fb_size, 0, 320);

	if (variant.color_depth_alias)
		state.builder.set_depth_image(0);
	else
		state.builder.set_depth_image(1u << 20u);

	state.builder.set_viewport({ 0, 0, 320, 240, 0, 1 });

	state.builder.set_enable_sample_quad(variant.sample_quad);

	if (variant.texture)
	{
		generate_random_tmem(rng, *state.reference, *state.gpu);
		state.builder.set_perspective(variant.perspective);

		if (variant.lod_frac)
		{
			state.builder.set_combiner_2cycle({
					                                  { RGBMulAdd::Texel1,   RGBMulSub::Texel0,   RGBMul::LODFrac,   RGBAdd::Texel0 },
					                                  { AlphaAddSub::Zero, AlphaAddSub::Zero, AlphaMul::Zero, AlphaAddSub::Texel0Alpha }
			                                  }, {
					                                  { RGBMulAdd::Zero,   RGBMulSub::Zero,   RGBMul::Zero,   RGBAdd::Combined },
					                                  { AlphaAddSub::Zero, AlphaAddSub::Zero, AlphaMul::Zero, AlphaAddSub::CombinedAlpha }
			                                  });
		}
		else
		{
			state.builder.set_combiner_1cycle({
					{ RGBMulAdd::Zero,   RGBMulSub::Zero,   RGBMul::Zero,   variant.pipelined_texel1 ? RGBAdd::Texel1 : RGBAdd::Texel0 },
					{ AlphaAddSub::Zero, AlphaAddSub::Zero, AlphaMul::Zero, AlphaAddSub::Texel0Alpha }
			});
		}
		state.builder.set_tex_lod_enable(variant.lod_frac);
	}
	else
	{
		state.builder.set_combiner_1cycle({
			{ RGBMulAdd::Zero,   RGBMulSub::Zero,   RGBMul::Zero,   RGBAdd::Shade },
			{ AlphaAddSub::Zero, AlphaAddSub::Zero, AlphaMul::Zero, AlphaAddSub::ShadeAlpha }
		});
	}

	state.builder.set_alpha_test(variant.alpha_test);
	state.builder.set_alpha_test_dither(variant.alpha_test_dither);
	state.builder.set_blend_color(0, 0, 0, 130);

	state.builder.set_enable_aa(variant.aa);
	state.builder.set_dither(variant.dither);
	state.builder.set_dither(variant.alpha_dither);
	state.builder.set_cvg_times_alpha(variant.cvg_times_alpha);
	state.builder.set_alpha_cvg_select(variant.alpha_cvg_select);
	state.builder.set_cycle_type(variant.cycle_type);
	state.builder.set_tex_lod_sharpen_enable(variant.lod_sharpen);
	state.builder.set_tex_lod_detail_enable(variant.lod_detail);
	state.builder.set_z_mode(variant.z_mode);
	state.builder.set_depth_test(variant.depth_compare);
	state.builder.set_coverage_mode(variant.coverage_mode);
	state.builder.set_image_read_enable(variant.image_read_enable);
	state.builder.set_color_on_coverage(variant.color_on_coverage);
	state.builder.set_enable_primitive_depth(variant.prim_depth);
	state.builder.set_enable_mid_texel(variant.mid_texel);
	state.builder.set_enable_convert_one(variant.convert_one);
	state.builder.set_enable_bilerp_cycle(0, variant.bilerp0);
	state.builder.set_enable_bilerp_cycle(1, variant.bilerp1);

	for (unsigned index = 0; index <= args.hi; index++)
	{
		clear_rdram(*state.reference);
		clear_rdram(*state.gpu);

		if (index & 2)
		{
			state.builder.set_scissor_subpixels(19, 14, 1162, 801,
			                                    variant.interlace, variant.interlace && bool(index & 1));
		}
		else
		{
			// Test binning behavior for FILL / COPY in particular.
			// FILL / COPY get coverage on edges, so the END coordinate of scissor should
			// get coverage, but not for CYCLE1/2.
			// This needs to be handled carefully when binning primitives.
			state.builder.set_scissor_subpixels(0, 0, 511 + ((index >> 3) & 7), 800,
			                                    variant.interlace, variant.interlace && bool(index & 1));
		}

		state.builder.set_fill_color(uint32_t(rng.rnd()));
		state.builder.set_depth_write(variant.depth && (rng.rnd() & 1) != 0);

		if (variant.prim_depth)
		{
			auto prim_z = uint16_t(rng.rnd());
			auto prim_dz = uint16_t(rng.rnd());
			state.builder.set_primitive_depth(prim_z, prim_dz);
		}

		if (variant.combiner_inputs)
		{
			state.builder.set_combiner_2cycle(
					{
							{
									generate_random_input(rng, RGBMulAdd::Zero),
									generate_random_input(rng, RGBMulSub::Zero),
									generate_random_input(rng, RGBMul::Zero),
									generate_random_input(rng, RGBAdd::Zero)
							},
							{
									generate_random_input(rng, AlphaAddSub::Zero),
									generate_random_input(rng, AlphaAddSub::Zero),
									generate_random_input(rng, AlphaMul::Zero),
									generate_random_input(rng, AlphaAddSub::Texel0Alpha)
							}
					},
					{
							{
									generate_random_input(rng, variant.cycle_type == CycleType::Cycle2 ? RGBMulAdd::Combined : RGBMulAdd::Zero),
									generate_random_input(rng, variant.cycle_type == CycleType::Cycle2 ? RGBMulSub::Combined : RGBMulSub::Zero),
									generate_random_input(rng, variant.cycle_type == CycleType::Cycle2 ? RGBMul::CombinedAlpha : RGBMul::Zero),
									generate_random_input(rng, variant.cycle_type == CycleType::Cycle2 ? RGBAdd::Combined : RGBAdd::Zero)
							},
							{
									generate_random_input(rng, variant.cycle_type == CycleType::Cycle2 ? AlphaAddSub::CombinedAlpha : AlphaAddSub::Zero),
									generate_random_input(rng, variant.cycle_type == CycleType::Cycle2 ? AlphaAddSub::CombinedAlpha : AlphaAddSub::Zero),
									generate_random_input(rng, variant.cycle_type == CycleType::Cycle2 ? AlphaMul::ShadeAlpha : AlphaMul::Zero),
									generate_random_input(rng, variant.cycle_type == CycleType::Cycle2 ? AlphaAddSub::CombinedAlpha : AlphaAddSub::Texel0Alpha)
							}
					}
			);

			state.builder.set_env_color(
					rng.rnd() & 0xff,
					rng.rnd() & 0xff,
					rng.rnd() & 0xff,
					rng.rnd() & 0xff);

			state.builder.set_key_r(
					rng.rnd() & 0xfff,
					rng.rnd() & 0xff,
					rng.rnd() & 0xff);

			state.builder.set_key_gb(
					rng.rnd() & 0xfff,
					rng.rnd() & 0xff,
					rng.rnd() & 0xff,
					rng.rnd() & 0xfff,
					rng.rnd() & 0xff,
					rng.rnd() & 0xff);
		}

		if (variant.combiner_inputs || variant.convert_one || !variant.bilerp0)
		{
			state.builder.set_convert(
					uint16_t(rng.rnd()),
					uint16_t(rng.rnd()),
					uint16_t(rng.rnd()),
					uint16_t(rng.rnd()),
					uint16_t(rng.rnd()),
					uint16_t(rng.rnd()));
		}

		state.builder.set_primitive_color(
				16, 0xaa,
				rng.rnd() & 0xff,
				rng.rnd() & 0xff,
				rng.rnd() & 0xff,
				rng.rnd() & 0xff);

		if (variant.blending)
		{
			if (variant.cycle_type == CycleType::Cycle2)
			{
				state.builder.set_blend_mode(0, BlendMode1A::PixelColor, BlendMode1B::ShadeAlpha,
				                             BlendMode2A::FogColor, BlendMode2B::InvPixelAlpha);
				state.builder.set_blend_mode(1, BlendMode1A::PixelColor, BlendMode1B::PixelAlpha,
				                             BlendMode2A::MemoryColor, BlendMode2B::InvPixelAlpha);
			}
			else
			{
				state.builder.set_blend_mode(0, BlendMode1A::PixelColor, BlendMode1B::ShadeAlpha,
				                             BlendMode2A::FogColor, BlendMode2B::InvPixelAlpha);
			}
			state.builder.set_enable_blend(true);
			state.builder.set_fog_color(
					rng.rnd() & 0xff,
					rng.rnd() & 0xff,
					rng.rnd() & 0xff,
					rng.rnd() & 0xff);
		}
		else if (variant.color_on_coverage)
		{
			state.builder.set_blend_mode(0, BlendMode1A::PixelColor, BlendMode1B::ShadeAlpha,
			                             BlendMode2A::MemoryColor, BlendMode2B::InvPixelAlpha);
			state.builder.set_enable_blend(false);
		}
		else if (variant.z_mode == ZMode::Interpenetrating)
		{
			// Tests blender shifters and special blender divider.
			state.builder.set_blend_mode(0, BlendMode1A::PixelColor, BlendMode1B::ShadeAlpha,
			                             BlendMode2A::MemoryColor, BlendMode2B::MemoryAlpha);
			state.builder.set_enable_blend(false);
		}
		else
		{
			state.builder.set_blend_mode(0, BlendMode1A::PixelColor, BlendMode1B::PixelAlpha,
			                             BlendMode2A::PixelColor, BlendMode2B::InvPixelAlpha);
			state.builder.set_enable_blend(false);
		}

		if (variant.texture)
		{
			TileMeta info = {};
			info.size = variant.texture_size;
			info.fmt = variant.texture_format;
			info.mask_s = rng.rnd() & 0xfu;
			info.mask_t = rng.rnd() & 0xfu;
			info.shift_s = rng.rnd() & 0xfu;
			info.shift_t = rng.rnd() & 0xfu;
			info.palette = rng.rnd() & 0xfu;
			info.stride = 24;
			info.offset = 8;

			if (rng.boolean())
				info.flags |= TILE_INFO_CLAMP_S_BIT;
			if (rng.boolean())
				info.flags |= TILE_INFO_CLAMP_T_BIT;
			if (rng.boolean())
				info.flags |= TILE_INFO_MIRROR_S_BIT;
			if (rng.boolean())
				info.flags |= TILE_INFO_MIRROR_T_BIT;
			state.builder.set_tlut(variant.tlut, variant.tlut_type);

			unsigned slo = rng.rnd() & 0xfu;
			unsigned tlo = rng.rnd() & 0xfu;
			unsigned width = 4 + (rng.rnd() & 0xffu);
			unsigned height = 4 + (rng.rnd() & 0xffu);

			for (unsigned i = 0; i < 8; i++)
			{
				state.builder.set_tile(i, info);
				state.builder.set_tile_size_subpixels(i, slo, tlo, width, height);
				info.offset += 16;
			}
		}

		if (variant.randomize_rdram)
			randomize_rdram(rng, *state.reference, *state.gpu);

		generate_random_input_primitive(rng, prim, variant.color, variant.depth, variant.force_flip);

		const auto generate_ym_offset = [&]() -> int { return int(rng.rnd() & 15) - 8; };

		if (index >= args.lo)
		{
			if (args.capture)
				state.device->begin_renderdoc_capture();

			for (unsigned j = 0; j < variant.primitive_count; j++)
			{
				int ym_offset = generate_ym_offset();

				if (variant.fill_rect)
				{
					uint16_t x, y, width, height;
					x = uint16_t(rng.rnd() & 63);
					y = uint16_t(rng.rnd() & 63);
					width = uint16_t(rng.rnd() & 2047);
					height = uint16_t(rng.rnd() & 2047);
					state.builder.fill_rectangle_subpixels(x, y, width, height);
				}
				else if (variant.tex_rect)
				{
					uint16_t x, y, width, height, s, t, dsdx, dtdy;
					x = uint16_t(rng.rnd() & 63);
					y = uint16_t(rng.rnd() & 63);
					width = uint16_t(rng.rnd() & 2047);
					height = uint16_t(rng.rnd() & 2047);
					s = uint16_t(rng.rnd());
					t = uint16_t(rng.rnd());
					dsdx = uint16_t(rng.rnd());
					dtdy = uint16_t(rng.rnd());
					if (rng.rnd() & 1)
						state.builder.tex_rect(3, x, y, width, height, s, t, dsdx, dtdy);
					else
						state.builder.tex_rect_flip(2, x, y, width, height, s, t, dsdx, dtdy);
				}
				else if (variant.ym_out_of_range)
					state.builder.draw_triangle_ym_out_of_range(prim, ym_offset);
				else
					state.builder.draw_triangle(prim);

				generate_random_input_primitive(rng, prim, variant.color, variant.depth, variant.force_flip);
			}

			state.builder.end_frame();
			if (args.capture)
				state.device->end_renderdoc_capture();

			if (!compare_rdram(*state.reference, *state.gpu))
			{
				LOGE("Rasterization conformance failed in iteration %u!\n", index);
				return false;
			}

			state.device->next_frame_context();
		}
		else
		{
			for (unsigned j = 0; j < variant.primitive_count; j++)
			{
				generate_ym_offset();
				generate_random_input_primitive(rng, prim, variant.color, variant.depth, variant.force_flip);
			}
		}

		if (args.verbose)
			LOGI("Iteration %u passed ...\n", index);
	}
	return true;
}

static bool run_conformance_load_tile(ReplayerState &state, const Arguments &args, unsigned width, unsigned height,
                                      uint32_t tmem_offset, uint32_t tmem_stride,
                                      uint32_t rdram_offset, Op op, TextureSize vram_size, TextureSize tile_size,
                                      uint32_t dxt, bool yuv)
{
	RNG rng;
	auto *tmem_reference = reinterpret_cast<uint16_t *>(state.reference->get_tmem());
	auto *tmem_gpu = reinterpret_cast<uint16_t *>(state.gpu->get_tmem());
	memset(tmem_reference, 0, 0x1000);
	memset(tmem_gpu, 0, 0x1000);

	if (op == Op::LoadTLut)
		height = 1;

	auto *rdram_reference = reinterpret_cast<uint32_t *>(state.reference->get_rdram());
	rdram_reference += 512 * 1024;
	auto *rdram_gpu = reinterpret_cast<uint32_t *>(state.gpu->get_rdram());
	rdram_gpu += 512 * 1024;

	for (unsigned i = 0; i < 64 * 1024; i++)
	{
		//auto v = uint32_t(rng.rnd());
		auto v = ((uint32_t(4 * i + 0) & 0xff) << 24) |
		         ((uint32_t(4 * i + 1) & 0xff) << 16) |
		         ((uint32_t(4 * i + 2) & 0xff) << 8) |
		         ((uint32_t(4 * i + 3) & 0xff) << 0);
		rdram_reference[i] = v;
		rdram_gpu[i] = v;
	}

	state.builder.set_texture_image(2 * 1024 * 1024 + rdram_offset, TextureFormat::RGBA, vram_size, width);

	TileMeta info;
	info.offset = tmem_offset;
	info.stride = tmem_stride;
	info.size = tile_size;
	info.fmt = yuv ? TextureFormat::YUV : TextureFormat::RGBA;
	state.builder.set_tile(0, info);
	if (op == Op::LoadTLut)
		state.builder.load_tlut(0, 1, 1, width, height);
	else if (op == Op::LoadTile)
		state.builder.load_tile_subpixels(0, (width & 3) + 8, (height & 3) + 8, (width << 2) | (height & 3), (height << 2) | (width & 3));
	else if (op == Op::LoadBlock)
		state.builder.load_block(0, 1, 3, width, dxt ? dxt : ((1 << 10) >> (height & 3)));
	state.combined->idle();
	state.device->next_frame_context();

	for (unsigned i = 0; i < 2048; i++)
	{
		auto sample_ref = tmem_reference[i ^ 1];
		auto sample_gpu = tmem_gpu[i ^ 1];
		if (sample_gpu != sample_ref)
		{
			LOGE("TMEM16[0x%x] differs! (reference: 0x%x, gpu: 0x%x)\n", i, sample_ref, sample_gpu);
			return false;
		}
	}
	return true;
}

static bool run_conformance_load_tile(ReplayerState &state, const Arguments &args,
                                      Op op, TextureSize vram_size, TextureSize tile_size, bool yuv)
{
	// Simple, aligned case.
	if (!run_conformance_load_tile(state, args, 32, 7, 0, 128, 0, op, vram_size, tile_size, 0, yuv))
	{
		LOG_FAILURE();
		return false;
	}

	if (tile_size == TextureSize::Bpp16)
	{
		// 4kB TMEM case.
		if (!run_conformance_load_tile(state, args, 64, 32, 0, 128, 0, op, vram_size, tile_size, 0, yuv))
		{
			LOG_FAILURE();
			return false;
		}
	}

	if (op == Op::LoadTile)
	{
		// TMEM wrap-around case.
		if (!run_conformance_load_tile(state, args, 128, 64, 0, 128 + 8, 0, op, vram_size, tile_size, 0, yuv))
		{
			LOG_FAILURE();
			return false;
		}

		// T overflow case.
		if (!run_conformance_load_tile(state, args, 4, 1024, 0, 8, 0, op, vram_size, tile_size, 0, yuv))
		{
			LOG_FAILURE();
			return false;
		}

		// Another T overflow case.
		if (!run_conformance_load_tile(state, args, 4, 1023, 0, 8, 0, op, vram_size, tile_size, 0, yuv))
		{
			LOG_FAILURE();
			return false;
		}
	}

	if (op == Op::LoadBlock)
	{
		if (tile_size == TextureSize::Bpp16 && !yuv)
		{
			// Test case where DxT is uneven.
			if (!run_conformance_load_tile(state, args, 1600, 1, 0, 0, 0, op, vram_size, tile_size, 103, yuv))
			{
				LOG_FAILURE();
				return false;
			}

			// Test case where DxT is uneven, now with a TMEM stride.
			if (!run_conformance_load_tile(state, args, 1600, 1, 0, 8, 0, op, vram_size, tile_size, 103, yuv))
			{
				LOG_FAILURE();
				return false;
			}
		}

		if ((vram_size == TextureSize::Bpp32 && tile_size == TextureSize::Bpp32 && !yuv) ||
		    (vram_size == TextureSize::Bpp16 && tile_size == TextureSize::Bpp16 && yuv))
		{
			// Test case where DxT is uneven and gets T wraparound within the 64-bit word.
			if (!run_conformance_load_tile(state, args, 799, 1, 0, 0, 0, op, vram_size, tile_size, 872, yuv))
			{
				LOG_FAILURE();
				return false;
			}

			// Batshit insanity, now with TMEM stride on top of it!
			if (!run_conformance_load_tile(state, args, 100, 1, 0, 32, 0, op, vram_size, tile_size, 872, yuv))
			{
				LOG_FAILURE();
				return false;
			}
		}
	}

	// Simple, aligned case, to high TMEM
	if (!run_conformance_load_tile(state, args, 32, 7, 0x800, 128, 0, op, vram_size, tile_size, 0, yuv))
	{
		LOG_FAILURE();
		return false;
	}

	// Simple, aligned case with TMEM offset.
	if (!run_conformance_load_tile(state, args, 32, 7, 128, 128, 0, op, vram_size, tile_size, 0, yuv))
	{
		LOG_FAILURE();
		return false;
	}

	// Unaligned case.
	if (!run_conformance_load_tile(state, args, 32, 9, 0, 128, 1, op, vram_size, tile_size, 0, yuv))
	{
		LOG_FAILURE();
		return false;
	}

	// Test stride == 0 cases.
	for (unsigned width = 9; width < 32; width++)
	{
		for (unsigned height = 1; height < 4; height++)
		{
			if (!run_conformance_load_tile(state, args, width, height, 0, 0, height & 3, op, vram_size, tile_size, 0, yuv))
			{
				LOG_FAILURE();
				return false;
			}
		}
	}

	// Test stride > width cases.
	for (unsigned width = 8; width < 32; width++)
	{
		if (!run_conformance_load_tile(state, args, width, 3, 0, 256, width & 3, op, vram_size, tile_size, 0, yuv))
		{
			LOG_FAILURE();
			return false;
		}
	}

	// Test stride < width cases.
	for (unsigned stride = 8; stride <= 24; stride += 8)
	{
		for (unsigned width = 8; width < 64; width++)
		{
			if (!run_conformance_load_tile(state, args, width, 3, 0, stride, 0, op, vram_size, tile_size, 0, yuv))
			{
				LOG_FAILURE();
				return false;
			}
		}
	}

	return true;
}

template <bool block, bool yuv = false>
static bool run_conformance_load_tile4(ReplayerState &state, const Arguments &args)
{
	if (!block)
	{
		if (!run_conformance_load_tile(state, args, block ? Op::LoadBlock : Op::LoadTile, TextureSize::Bpp8,
		                               TextureSize::Bpp4, yuv))
		{
			LOG_FAILURE();
			return false;
		}
	}

	return true;
}

template <bool block, bool yuv = false>
static bool run_conformance_load_tile8(ReplayerState &state, const Arguments &args)
{
	if (!run_conformance_load_tile(state, args, block ? Op::LoadBlock : Op::LoadTile, TextureSize::Bpp8, TextureSize::Bpp8, yuv))
	{
		LOG_FAILURE();
		return false;
	}

	if (!block)
	{
		if (!run_conformance_load_tile(state, args, block ? Op::LoadBlock : Op::LoadTile, TextureSize::Bpp16,
		                               TextureSize::Bpp8, yuv))
		{
			LOG_FAILURE();
			return false;
		}
	}

	return true;
}

template <bool block, bool yuv = false>
static bool run_conformance_load_tile16(ReplayerState &state, const Arguments &args)
{
	if (!run_conformance_load_tile(state, args, block ? Op::LoadBlock : Op::LoadTile, TextureSize::Bpp16, TextureSize::Bpp16, yuv))
	{
		LOG_FAILURE();
		return false;
	}

	if (!yuv)
	{
		if (!run_conformance_load_tile(state, args, block ? Op::LoadBlock : Op::LoadTile, TextureSize::Bpp8, TextureSize::Bpp16, yuv))
		{
			LOG_FAILURE();
			return false;
		}
	}

	if (!block && !yuv)
	{
		if (!run_conformance_load_tile(state, args, block ? Op::LoadBlock : Op::LoadTile, TextureSize::Bpp32,
		                               TextureSize::Bpp16, yuv))
		{
			LOG_FAILURE();
			return false;
		}
	}

	return true;
}

static bool run_conformance_load_tlut4(ReplayerState &state, const Arguments &args)
{
	if (!run_conformance_load_tile(state, args, Op::LoadTLut, TextureSize::Bpp16, TextureSize::Bpp4, false))
	{
		LOG_FAILURE();
		return false;
	}

	if (!run_conformance_load_tile(state, args, Op::LoadTLut, TextureSize::Bpp8, TextureSize::Bpp4, false))
	{
		LOG_FAILURE();
		return false;
	}

	if (!run_conformance_load_tile(state, args, Op::LoadTLut, TextureSize::Bpp32, TextureSize::Bpp4, false))
	{
		LOG_FAILURE();
		return false;
	}

	return true;
}

static bool run_conformance_load_tlut8(ReplayerState &state, const Arguments &args)
{
	if (!run_conformance_load_tile(state, args, Op::LoadTLut, TextureSize::Bpp16, TextureSize::Bpp8, false))
	{
		LOG_FAILURE();
		return false;
	}

	if (!run_conformance_load_tile(state, args, Op::LoadTLut, TextureSize::Bpp8, TextureSize::Bpp8, false))
	{
		LOG_FAILURE();
		return false;
	}

	if (!run_conformance_load_tile(state, args, Op::LoadTLut, TextureSize::Bpp32, TextureSize::Bpp8, false))
	{
		LOG_FAILURE();
		return false;
	}

	return true;
}

static bool run_conformance_load_tlut16(ReplayerState &state, const Arguments &args)
{
	if (!run_conformance_load_tile(state, args, Op::LoadTLut, TextureSize::Bpp16, TextureSize::Bpp16, false))
	{
		LOG_FAILURE();
		return false;
	}

	if (!run_conformance_load_tile(state, args, Op::LoadTLut, TextureSize::Bpp8, TextureSize::Bpp16, false))
	{
		LOG_FAILURE();
		return false;
	}

	if (!run_conformance_load_tile(state, args, Op::LoadTLut, TextureSize::Bpp32, TextureSize::Bpp16, false))
	{
		LOG_FAILURE();
		return false;
	}

	return true;
}

template <bool block, bool yuv = false>
static bool run_conformance_load_tile32(ReplayerState &state, const Arguments &args)
{
	if (!run_conformance_load_tile(state, args, block ? Op::LoadBlock : Op::LoadTile, TextureSize::Bpp32, TextureSize::Bpp32, false))
	{
		LOG_FAILURE();
		return false;
	}

	if (!run_conformance_load_tile(state, args, block ? Op::LoadBlock : Op::LoadTile, TextureSize::Bpp16, TextureSize::Bpp32, false))
	{
		LOG_FAILURE();
		return false;
	}

	if (!run_conformance_load_tile(state, args, block ? Op::LoadBlock : Op::LoadTile, TextureSize::Bpp8, TextureSize::Bpp32, false))
	{
		LOG_FAILURE();
		return false;
	}

	return true;
}

static void print_help()
{
	LOGE("Usage: rdp-conformance\n"
	     "\t[--suite-glob <suite>]\n"
	     "\t[--suite <suite>]\n"
	     "\t[--range <lo> <hi>]\n"
	     "\t[--capture]\n"
	     "\t[--list-suites]\n"
	     "\t[--verbose]\n"
	);
}

static int main_inner(int argc, char **argv)
{
	Arguments cli_args;
	bool list_suites = false;

	Util::CLICallbacks cbs;
	cbs.add("--help", [](Util::CLIParser &parser) { print_help(); parser.end(); });
	cbs.add("--suite-glob", [&](Util::CLIParser &parser) { cli_args.suite_glob = parser.next_string(); });
	cbs.add("--suite", [&](Util::CLIParser &parser) { cli_args.suite = parser.next_string(); });
	cbs.add("--verbose", [&](Util::CLIParser &) { cli_args.verbose = true; });
	cbs.add("--range", [&](Util::CLIParser &parser) {
		cli_args.lo = parser.next_uint();
		cli_args.hi = parser.next_uint();
	});
	cbs.add("--capture", [&](Util::CLIParser &) {
		cli_args.capture = Vulkan::Device::init_renderdoc_capture();
	});
	cbs.add("--list-suites", [&](Util::CLIParser &) { list_suites = true; });
	Util::CLIParser parser(std::move(cbs), argc - 1, argv + 1);

	if (!parser.parse())
	{
		print_help();
		return EXIT_FAILURE;
	}
	else if (parser.is_ended_state())
		return EXIT_SUCCESS;

	struct Suite
	{
		std::string name;
		bool (*func)(ReplayerState &state, const Arguments &args);
	};
	std::vector<Suite> suites;

	suites.push_back({ "fill-ym-out-of-range", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.cycle_type = CycleType::Fill;
		variant.fb_size = TextureSize::Bpp16;
		variant.ym_out_of_range = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "fill-8", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.cycle_type = CycleType::Fill;
		variant.fb_size = TextureSize::Bpp8;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "fill-16", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.cycle_type = CycleType::Fill;
		variant.fb_size = TextureSize::Bpp16;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "fill-16-interlace", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.cycle_type = CycleType::Fill;
		variant.fb_size = TextureSize::Bpp16;
		variant.interlace = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "fill-16-ia", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.cycle_type = CycleType::Fill;
		variant.fb_size = TextureSize::Bpp16;
		variant.texture_format = TextureFormat::IA;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "fill-32", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.cycle_type = CycleType::Fill;
		variant.fb_size = TextureSize::Bpp32;
		return run_conformance_rasterization(state, args, variant);
	}});

#define COPY_TEST(name, tmem_bpp, fb_bpp, atest, lut) \
	suites.push_back({ "copy-" #name, [](ReplayerState &state, const Arguments &args) -> bool { \
		RasterizationTestVariant variant = {}; \
		variant.cycle_type = CycleType::Copy; \
		variant.texture = true; \
		variant.force_flip = TextureSize::fb_bpp != TextureSize::Bpp8; \
		variant.texture_format = TextureFormat::RGBA; \
		variant.texture_size = TextureSize::tmem_bpp; \
		variant.randomize_rdram = TextureSize::fb_bpp == TextureSize::Bpp4; \
		variant.fb_size = TextureSize::fb_bpp; \
		variant.alpha_test = atest; \
		variant.tlut = lut; \
		return run_conformance_rasterization(state, args, variant); \
	}})
	COPY_TEST(32bpp-fb8, Bpp32, Bpp8, false, false);
	COPY_TEST(32bpp-fb16, Bpp32, Bpp16, false, false);

	COPY_TEST(4bpp-fb8, Bpp4, Bpp8, false, false);
	COPY_TEST(4bpp-fb16, Bpp4, Bpp16, false, false);

	COPY_TEST(4bpp-fb4, Bpp4, Bpp4, false, false);
	COPY_TEST(8bpp-fb4, Bpp8, Bpp4, false, false);
	COPY_TEST(16bpp-fb4, Bpp16, Bpp4, false, false);
	COPY_TEST(32bpp-fb4, Bpp32, Bpp4, false, false);

	COPY_TEST(8bpp-fb8, Bpp8, Bpp8, false, false);
	COPY_TEST(8bpp-fb16, Bpp8, Bpp16, false, false);
	COPY_TEST(16bpp-fb8, Bpp16, Bpp8, false, false);
	COPY_TEST(16bpp-fb16, Bpp16, Bpp16, false, false);

	COPY_TEST(4bpp-fb16-tlut, Bpp4, Bpp16, false, true);
	COPY_TEST(8bpp-fb16-tlut, Bpp8, Bpp16, false, true);
	COPY_TEST(16bpp-fb16-tlut, Bpp16, Bpp16, false, true);

	COPY_TEST(16bpp-fb16-alpha-test, Bpp16, Bpp16, true, false);

	suites.push_back({ "fill-rect", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.fill_rect = true;
		variant.cycle_type = CycleType::Fill;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "tex-rect", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.tex_rect = true;
		variant.texture = true;
		variant.color = true;
		variant.cycle_type = CycleType::Copy;
		variant.texture_size = TextureSize::Bpp16;
		variant.fb_size = TextureSize::Bpp16;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "rasterization-noaa", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "rasterization-aa", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.aa = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "rasterization-interlace-aa", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.aa = true;
		variant.interlace = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "interpolation-color", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "interpolation-depth", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.depth = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "interpolation-color-depth", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.depth = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "combiner-1cycle", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.depth = true;
		variant.combiner_inputs = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "combiner-2cycle", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.depth = true;
		variant.combiner_inputs = true;
		variant.cycle_type = CycleType::Cycle2;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "combiner-2cycle-alpha-test-color", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.depth = true;
		variant.cycle_type = CycleType::Cycle2;
		variant.alpha_test = true;
		variant.alpha_dither = AlphaDitherMode::Pattern;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "combiner-2cycle-alpha-test-texture", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.texture = true;
		variant.color = true;
		variant.depth = true;
		variant.cycle_type = CycleType::Cycle2;
		variant.alpha_test = true;
		variant.alpha_dither = AlphaDitherMode::Pattern;
		return run_conformance_rasterization(state, args, variant);
	}});

#define BLENDER_FOG_TEST(name, fmt, size) \
	suites.push_back({ "blender-fog-color-1cycle-" #name, [](ReplayerState &state, const Arguments &args) -> bool { \
		RasterizationTestVariant variant = {}; \
		variant.color = true; \
		variant.depth = true; \
		variant.blending = true; \
		variant.dither = RGBDitherMode::Magic; \
		variant.alpha_dither = AlphaDitherMode::Pattern; \
		variant.fb_fmt = TextureFormat::fmt; \
		variant.fb_size = TextureSize::size; \
		return run_conformance_rasterization(state, args, variant); \
	}})
	BLENDER_FOG_TEST(i4, RGBA, Bpp4);
	BLENDER_FOG_TEST(i8, RGBA, Bpp8);
	BLENDER_FOG_TEST(rgba5551, RGBA, Bpp16);
	BLENDER_FOG_TEST(ia88, IA, Bpp16);
	BLENDER_FOG_TEST(rgba8888, RGBA, Bpp32);

#define BLENDER_FOG_TEST2(name, fmt, size) \
	suites.push_back({ "blender-fog-color-2cycle-" #name, [](ReplayerState &state, const Arguments &args) -> bool { \
		RasterizationTestVariant variant = {}; \
		variant.color = true; \
		variant.depth = true; \
		variant.blending = true; \
		variant.dither = RGBDitherMode::Magic; \
		variant.alpha_dither = AlphaDitherMode::Pattern; \
		variant.cycle_type = CycleType::Cycle2; \
		variant.randomize_rdram = true; \
		variant.fb_fmt = TextureFormat::fmt; \
		variant.fb_size = TextureSize::size; \
		return run_conformance_rasterization(state, args, variant); \
	}})
	BLENDER_FOG_TEST2(i4, RGBA, Bpp4);
	BLENDER_FOG_TEST2(i8, RGBA, Bpp8);
	BLENDER_FOG_TEST2(rgba5551, RGBA, Bpp16);
	BLENDER_FOG_TEST2(ia88, IA, Bpp16);
	BLENDER_FOG_TEST2(rgba8888, RGBA, Bpp32);

	suites.push_back({ "interpolation-color-depth-cvg-times-alpha", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.depth = true;
		variant.cvg_times_alpha = true;
		variant.alpha_test = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "interpolation-color-depth-aa-cvg-times-alpha", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.depth = true;
		variant.aa = true;
		variant.cvg_times_alpha = true;
		variant.alpha_test = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "interpolation-color-depth-aa-cvg-times-alpha-alpha-cvg-select", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.depth = true;
		variant.aa = true;
		variant.cvg_times_alpha = true;
		variant.alpha_test = true;
		variant.alpha_cvg_select = true;
		return run_conformance_rasterization(state, args, variant);
	}});
	suites.push_back({ "interpolation-color-depth-aa-alpha-cvg-select", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.depth = true;
		variant.aa = true;
		variant.cvg_times_alpha = false;
		variant.alpha_test = true;
		variant.alpha_cvg_select = true;
		return run_conformance_rasterization(state, args, variant);
	}});

#define COVERAGE_MEMORY_TEST(name, mode, image_read_en) \
	suites.push_back({ "coverage-" #name, [](ReplayerState &state, const Arguments &args) -> bool { \
		RasterizationTestVariant variant = {}; \
		variant.color = true; \
		variant.aa = true; \
		variant.coverage_mode = mode; \
		variant.image_read_enable = image_read_en; \
		variant.randomize_rdram = true; \
		return run_conformance_rasterization(state, args, variant); \
	}})
	COVERAGE_MEMORY_TEST(clamp, CoverageMode::Clamp, false);
	COVERAGE_MEMORY_TEST(wrap, CoverageMode::Wrap, false);
	COVERAGE_MEMORY_TEST(zap, CoverageMode::Zap, false);
	COVERAGE_MEMORY_TEST(save, CoverageMode::Save, false);
	COVERAGE_MEMORY_TEST(clamp-image-read, CoverageMode::Clamp, true);
	COVERAGE_MEMORY_TEST(wrap-image-read, CoverageMode::Wrap, true);
	COVERAGE_MEMORY_TEST(zap-image-read, CoverageMode::Zap, true);
	COVERAGE_MEMORY_TEST(save-image-read, CoverageMode::Save, true);

	suites.push_back({ "color-on-coverage", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.aa = true;
		variant.coverage_mode = CoverageMode::Wrap;
		variant.image_read_enable = true;
		variant.randomize_rdram = true;
		variant.color_on_coverage = true;
		return run_conformance_rasterization(state, args, variant);
	}});

#define DEPTH_COMPARE_TEST(name, mode, prim_d) \
	suites.push_back({ "depth-compare-" #name, [](ReplayerState &state, const Arguments &args) -> bool { \
		RasterizationTestVariant variant = {}; \
		variant.color = true; \
		variant.depth = true; \
		variant.aa = true; \
		variant.depth_compare = true; \
		variant.randomize_rdram = true; \
		variant.image_read_enable = true; \
		variant.z_mode = ZMode::mode; \
		variant.prim_depth = prim_d; \
		return run_conformance_rasterization(state, args, variant); \
	}})
	DEPTH_COMPARE_TEST(opaque, Opaque, false);
	DEPTH_COMPARE_TEST(interpenetrating, Interpenetrating, false);
	DEPTH_COMPARE_TEST(transparent, Transparent, false);
	DEPTH_COMPARE_TEST(decal, Decal, false);
	DEPTH_COMPARE_TEST(opaque-prim-depth, Opaque, true);
	DEPTH_COMPARE_TEST(interpenetrating-prim-depth, Interpenetrating, true);
	DEPTH_COMPARE_TEST(transparent-prim-depth, Transparent, true);
	DEPTH_COMPARE_TEST(decal-prim-depth, Decal, true);

#define DITHER_TEST(name, rgb, alpha, test_dither) \
	suites.push_back({ "interpolation-color-depth-alpha-test-dither-" #name, [](ReplayerState &state, const Arguments &args) -> bool { \
		RasterizationTestVariant variant = {}; \
		variant.color = true; \
		variant.depth = true; \
		variant.dither = RGBDitherMode::rgb; \
		variant.alpha_dither = AlphaDitherMode::alpha; \
		variant.alpha_test = true; \
		variant.alpha_test_dither = test_dither; \
		return run_conformance_rasterization(state, args, variant); \
	}})
	DITHER_TEST(off, Off, Off, false);
	DITHER_TEST(test, Off, Off, true);
	DITHER_TEST(all, Noise, Noise, true);
	DITHER_TEST(magic-pattern, Magic, Pattern, false);
	DITHER_TEST(magic-flip, Magic, InvPattern, false);
	DITHER_TEST(magic-noise, Magic, Noise, false);
	DITHER_TEST(bayer-pattern, Bayer, Pattern, false);
	DITHER_TEST(bayer-flip, Bayer, InvPattern, false);
	DITHER_TEST(bayer-noise, Bayer, Noise, false);
	DITHER_TEST(off-pattern, Off, Pattern, false);
	DITHER_TEST(off-flip, Off, InvPattern, false);
	DITHER_TEST(off-noise, Off, Noise, false);
	DITHER_TEST(noise-off, Noise, Off, false);
	DITHER_TEST(noise-pattern, Noise, Pattern, false);
	DITHER_TEST(noise-flip, Noise, InvPattern, false);
	DITHER_TEST(noise-noise, Noise, Noise, false);

	suites.push_back({ "interpolation-color-texture-2cycle-lod-frac", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.texture_format = TextureFormat::RGBA;
		variant.texture_size = TextureSize::Bpp16;
		variant.lod_frac = true;
		variant.cycle_type = CycleType::Cycle2;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-perspective-2cycle-lod-frac", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.perspective = true;
		variant.texture_format = TextureFormat::RGBA;
		variant.texture_size = TextureSize::Bpp16;
		variant.lod_frac = true;
		variant.cycle_type = CycleType::Cycle2;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-perspective-2cycle-lod-frac-sharpen", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.perspective = true;
		variant.texture_format = TextureFormat::RGBA;
		variant.texture_size = TextureSize::Bpp16;
		variant.lod_frac = true;
		variant.cycle_type = CycleType::Cycle2;
		variant.lod_sharpen = true;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-perspective-2cycle-lod-frac-detail", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.perspective = true;
		variant.texture_format = TextureFormat::RGBA;
		variant.texture_size = TextureSize::Bpp16;
		variant.lod_frac = true;
		variant.cycle_type = CycleType::Cycle2;
		variant.lod_detail = true;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-perspective-2cycle-lod-frac-sharpen-detail", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.perspective = true;
		variant.texture_format = TextureFormat::RGBA;
		variant.texture_size = TextureSize::Bpp16;
		variant.lod_frac = true;
		variant.cycle_type = CycleType::Cycle2;
		variant.lod_detail = true;
		variant.lod_sharpen = true;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "rasterization-many-primitives", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.primitive_count = 5 * 1024;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "rasterization-many-primitives-alias", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.depth = true;
		variant.primitive_count = 1024;
		variant.color_depth_alias = true;
		variant.depth_compare = true;
		variant.randomize_rdram = true;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-pipelined-texel1", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.pipelined_texel1 = true;
		variant.texture_size = TextureSize::Bpp16;
		variant.texture_format = TextureFormat::RGBA;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-pipelined-texel1-perspective", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.pipelined_texel1 = true;
		variant.texture_size = TextureSize::Bpp16;
		variant.texture_format = TextureFormat::RGBA;
		variant.perspective = true;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-2cycle-convert-bilerp", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.texture_size = TextureSize::Bpp16;
		variant.texture_format = TextureFormat::RGBA;
		variant.cycle_type = CycleType::Cycle2;
		variant.convert_one = true;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-2cycle-convert-factors", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.texture_size = TextureSize::Bpp16;
		variant.texture_format = TextureFormat::RGBA;
		variant.cycle_type = CycleType::Cycle2;
		variant.convert_one = true;
		variant.bilerp1 = false;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-2cycle-implicit-convert-factors", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.texture_size = TextureSize::Bpp16;
		variant.texture_format = TextureFormat::RGBA;
		variant.cycle_type = CycleType::Cycle2;
		variant.sample_quad = false;
		variant.convert_one = false;
		variant.bilerp1 = false;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-2cycle-implicit-convert-factors-bilerp", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.texture_size = TextureSize::Bpp16;
		variant.texture_format = TextureFormat::RGBA;
		variant.cycle_type = CycleType::Cycle2;
		variant.sample_quad = false;
		variant.convert_one = false;
		variant.bilerp1 = true;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "interpolation-color-texture-yuv16-nearest", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.texture_size = TextureSize::Bpp16;
		variant.texture_format = TextureFormat::YUV;
		variant.cycle_type = CycleType::Cycle1;
		variant.bilerp1 = false;
		variant.bilerp0 = false;
		variant.sample_quad = false;
		return run_conformance_rasterization(state, args, variant);
	}});

#define TEXTURE_TEST_MID_TEXEL(name, fmt, size, tlut_enable, tlut_ia_type, sample_q, mid) \
	suites.push_back({ "interpolation-color-texture-" #name, [](ReplayerState &state, const Arguments &args) -> bool { \
		RasterizationTestVariant variant = {}; \
		variant.color = true; \
		variant.texture = true; \
		variant.texture_format = TextureFormat::fmt; \
		variant.texture_size = TextureSize::size; \
		variant.tlut = tlut_enable; \
		variant.tlut_type = tlut_ia_type; \
		variant.sample_quad = sample_q; \
		variant.mid_texel = mid; \
		return run_conformance_rasterization(state, args, variant); \
	}})
#define TEXTURE_TEST(name, fmt, size, tlut_enable, tlut_ia_type, sample_q) \
	TEXTURE_TEST_MID_TEXEL(name, fmt, size, tlut_enable, tlut_ia_type, sample_q, false)

	TEXTURE_TEST(rgba4, RGBA, Bpp4, false, false, true);
	TEXTURE_TEST(rgba8, RGBA, Bpp8, false, false, true);
	TEXTURE_TEST(rgba16, RGBA, Bpp16, false, false, true);
	TEXTURE_TEST(rgba32, RGBA, Bpp32, false, false, true);

	TEXTURE_TEST(yuv16, YUV, Bpp16, false, false, true);

	TEXTURE_TEST_MID_TEXEL(rgba16-mid-texel, RGBA, Bpp16, false, false, true, true);

	TEXTURE_TEST(rgba4-nearest, RGBA, Bpp4, false, false, false);
	TEXTURE_TEST(rgba8-nearest, RGBA, Bpp8, false, false, false);
	TEXTURE_TEST(rgba16-nearest, RGBA, Bpp16, false, false, false);
	TEXTURE_TEST(rgba32-nearest, RGBA, Bpp32, false, false, false);

	TEXTURE_TEST(ci4, CI, Bpp4, false, false, true);
	TEXTURE_TEST(ci8, CI, Bpp8, false, false, true);
	TEXTURE_TEST(ci16, CI, Bpp16, false, false, true);
	TEXTURE_TEST(ci32, CI, Bpp32, false, false, true);
	TEXTURE_TEST(ia4, IA, Bpp4, false, false, true);
	TEXTURE_TEST(ia8, IA, Bpp8, false, false, true);
	TEXTURE_TEST(ia16, IA, Bpp16, false, false, true);
	TEXTURE_TEST(ia32, IA, Bpp32, false, false, true);
	TEXTURE_TEST(i4, I, Bpp4, false, false, true);
	TEXTURE_TEST(i8, I, Bpp8, false, false, true);
	TEXTURE_TEST(i16, I, Bpp16, false, false, true);
	TEXTURE_TEST(i32, I, Bpp32, false, false, true);

	TEXTURE_TEST(ci4-tlut, CI, Bpp4, true, false, true);
	TEXTURE_TEST(ci8-tlut, CI, Bpp8, true, false, true);
	TEXTURE_TEST(ci16-tlut, CI, Bpp16, true, false, true);
	TEXTURE_TEST(ci32-tlut, CI, Bpp32, true, false, true);
	TEXTURE_TEST(ia4-tlut, IA, Bpp4, true, false, true);
	TEXTURE_TEST(ia8-tlut, IA, Bpp8, true, false, true);
	TEXTURE_TEST(ia16-tlut, IA, Bpp16, true, false, true);
	TEXTURE_TEST(ia32-tlut, IA, Bpp32, true, false, true);
	TEXTURE_TEST(i4-tlut, I, Bpp4, true, false, true);
	TEXTURE_TEST(i8-tlut, I, Bpp8, true, false, true);
	TEXTURE_TEST(i16-tlut, I, Bpp16, true, false, true);
	TEXTURE_TEST(i32-tlut, I, Bpp32, true, false, true);
	TEXTURE_TEST(rgba4-tlut, RGBA, Bpp4, true, false, true);
	TEXTURE_TEST(rgba8-tlut, RGBA, Bpp8, true, false, true);
	TEXTURE_TEST(rgba16-tlut, RGBA, Bpp16, true, false, true);
	TEXTURE_TEST(rgba32-tlut, RGBA, Bpp32, true, false, true);

	TEXTURE_TEST(ci4-tlut-ia16, CI, Bpp4, true, true, true);
	TEXTURE_TEST(ci8-tlut-ia16, CI, Bpp8, true, true, true);
	TEXTURE_TEST(ci16-tlut-ia16, CI, Bpp16, true, true, true);
	TEXTURE_TEST(ci32-tlut-ia16, CI, Bpp32, true, true, true);

	suites.push_back({ "interpolation-color-texture-perspective", [](ReplayerState &state, const Arguments &args) -> bool {
		RasterizationTestVariant variant = {};
		variant.color = true;
		variant.texture = true;
		variant.perspective = true;
		variant.texture_size = TextureSize::Bpp16;
		return run_conformance_rasterization(state, args, variant);
	}});

	suites.push_back({ "texture-load-tile-16-yuv", run_conformance_load_tile16<false, true> });
	suites.push_back({ "texture-load-block-16-yuv", run_conformance_load_tile16<true, true> });

	suites.push_back({ "texture-load-tile-4", run_conformance_load_tile4<false> });
	suites.push_back({ "texture-load-tile-8", run_conformance_load_tile8<false> });
	suites.push_back({ "texture-load-tile-16", run_conformance_load_tile16<false> });
	suites.push_back({ "texture-load-tile-32", run_conformance_load_tile32<false> });
	//suites.push_back({ "texture-load-block-4", run_conformance_load_tile4<true> });
	suites.push_back({ "texture-load-block-8", run_conformance_load_tile8<true> });
	suites.push_back({ "texture-load-block-16", run_conformance_load_tile16<true> });
	suites.push_back({ "texture-load-block-32", run_conformance_load_tile32<true> });
	suites.push_back({ "texture-load-tlut-4", run_conformance_load_tlut4 });
	suites.push_back({ "texture-load-tlut-8", run_conformance_load_tlut8 });
	suites.push_back({ "texture-load-tlut-16", run_conformance_load_tlut16 });

	if (list_suites)
	{
		for (auto &suite : suites)
			LOGI("Suite: %s\n", suite.name.c_str());
		return EXIT_SUCCESS;
	}

	{
		ReplayerState state;
		if (!state.init())
			return EXIT_FAILURE;

		bool did_work = false;
		for (auto &suite : suites)
		{
			bool cmp;
			if (!cli_args.suite.empty())
				cmp = suite_compare(suite.name, cli_args.suite);
			else
				cmp = suite_compare_glob(suite.name, cli_args.suite_glob);

			if (cmp)
			{
				did_work = true;
				LOGI("\n");
				LOGI("================================================\n");
				LOGI("Running suite: %s\n", suite.name.c_str());
				LOGI("------------------------------------------------\n");

				if (!suite.func(state, cli_args))
				{
					LOGE(" ... Suite failed.\n");
					return EXIT_FAILURE;
				}
				else
					LOGI("====== PASSED ======\n");

				LOGI("\n\n");
			}
			else
				LOGI("Skipping suite: %s\n", suite.name.c_str());
		}

		if (!did_work)
		{
			LOGE("No suite matches.\n");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	Granite::Global::init();
	setup_filesystems();
	int ret = main_inner(argc, argv);
	Granite::Global::deinit();
	return ret;
}

