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
#include "conformance_utils.hpp"
#include "global_managers.hpp"
#include "cli_parser.hpp"
#include "global_managers_init.hpp"

using namespace RDP;

struct Arguments
{
	std::string suite_glob;
	std::string suite;
	unsigned lo = 0;
	unsigned hi = 32;
	bool verbose = false;
	bool capture = false;
};

static void print_help()
{
	LOGE("Usage: vi-conformance\n"
	     "\t[--suite-glob <suite>]\n"
	     "\t[--suite <suite>]\n"
	     "\t[--range <lo> <hi>]\n"
	     "\t[--capture]\n"
	     "\t[--list-suites]\n"
	     "\t[--verbose]\n"
	);
}

struct VITestVariant
{
	VIControlFlagBits aa = VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT;
	VIControlFlagBits fmt = VI_CONTROL_TYPE_RGBA5551_BIT;

	uint32_t x_scale = 1024;
	uint32_t y_scale = 1024;
	uint32_t x_bias = 0;
	uint32_t y_bias = 0;
	bool pal = false;
	bool randomize_scale_bias = false;
	bool randomize_start = false;
	bool divot = false;
	bool dither_filter = false;
	bool gamma = false;
	bool gamma_dither = false;
	bool serrate = false;
};

static void set_default_vi_registers(ReplayerDriver &state, const VITestVariant &variant)
{
	state.set_vi_register(VIRegister::Control,
	                      variant.aa |
	                      variant.fmt |
	                      (variant.divot ? VI_CONTROL_DIVOT_ENABLE_BIT : 0) |
	                      (variant.dither_filter ? VI_CONTROL_DITHER_FILTER_ENABLE_BIT : 0) |
	                      (variant.gamma ? VI_CONTROL_GAMMA_ENABLE_BIT : 0) |
	                      (variant.gamma_dither ? VI_CONTROL_GAMMA_DITHER_ENABLE_BIT : 0) |
	                      (variant.serrate ? VI_CONTROL_SERRATE_BIT : 0));

	state.set_vi_register(VIRegister::Origin, 567123);
	state.set_vi_register(VIRegister::Width, 100);
	state.set_vi_register(VIRegister::VSync, variant.pal ? VI_V_SYNC_PAL : VI_V_SYNC_NTSC);
	state.set_vi_register(VIRegister::VStart,
	                      make_vi_start_register(variant.pal ? VI_V_OFFSET_PAL : VI_V_OFFSET_NTSC,
	                                             (variant.pal ? VI_V_OFFSET_PAL : VI_V_OFFSET_NTSC) + 224 * 2));
	state.set_vi_register(VIRegister::XScale, make_vi_scale_register(variant.x_scale, variant.x_bias));
	state.set_vi_register(VIRegister::YScale, make_vi_scale_register(variant.y_scale, variant.y_bias));

	// Ensure persistent state is cleared out between tests.
	state.set_vi_register(VIRegister::HStart,
	                      make_vi_start_register(640, 0));
	state.end_frame();

	state.set_vi_register(VIRegister::HStart,
	                      make_vi_start_register(variant.pal ? VI_H_OFFSET_PAL : VI_H_OFFSET_NTSC,
	                                             (variant.pal ? VI_H_OFFSET_PAL : VI_H_OFFSET_NTSC) + 640));
}

static void set_default_vi_registers(ReplayerState &state, const VITestVariant &variant)
{
	set_default_vi_registers(*state.combined, variant);
	set_default_vi_registers(*state.gpu_scaled, variant);
}

static bool run_conformance_vi(ReplayerState &state, const Arguments &args, const VITestVariant &variant)
{
	set_default_vi_registers(state, variant);

	RNG rng;
	for (unsigned i = 0; i <= args.hi; i++)
	{
		randomize_rdram(rng, *state.reference, *state.gpu);
		state.combined->set_vi_register(VIRegister::VCurrentLine, uint32_t(variant.serrate) & (i & 1u));

		if (variant.randomize_scale_bias)
		{
			auto x_scale = uint32_t(rng.rnd());
			auto y_scale = uint32_t(rng.rnd());
			auto x_bias = uint32_t(rng.rnd());
			auto y_bias = uint32_t(rng.rnd());

			state.combined->set_vi_register(VIRegister::XScale, make_vi_scale_register(x_scale, x_bias));
			state.combined->set_vi_register(VIRegister::YScale, make_vi_scale_register(y_scale, y_bias));
		}

		if (variant.randomize_start)
		{
			auto h_start = uint32_t(rng.rnd());
			auto v_start = uint32_t(rng.rnd());
			auto h_end = uint32_t(rng.rnd());
			auto v_end = uint32_t(rng.rnd());
			state.combined->set_vi_register(VIRegister::HStart, make_vi_start_register(h_start, h_end));
			state.combined->set_vi_register(VIRegister::VStart, make_vi_start_register(v_start, v_end));
		}

		if (i >= args.lo)
		{
			if (args.capture)
				state.device->begin_renderdoc_capture();
			state.combined->end_frame();
			if (args.capture)
				state.device->end_renderdoc_capture();

			if (!compare_image(state.iface.scanout_result[0], state.iface.widths[0], state.iface.heights[0],
			                   state.iface.scanout_result[1], state.iface.widths[1], state.iface.heights[1]))
			{
				LOGE("VI conformance failed in iteration %u!\n", i);
				return false;
			}

			state.device->next_frame_context();
		}

		if (args.verbose)
			LOGI("Iteration %u passed ...\n", i);
	}
	return true;
}

static bool run_per_scanline_xh_vi(ReplayerState &state, const Arguments &args, bool upscale, bool crop)
{
	// Reference does not support any of this, so we test more directly.
	// TODO: Verify this behavior against hardware.
	VITestVariant variant;
	variant.fmt = VI_CONTROL_TYPE_RGBA8888_BIT;
	variant.aa = VI_CONTROL_AA_MODE_RESAMP_ONLY_BIT;
	set_default_vi_registers(state, variant);
	auto &gpu = upscale ? *state.gpu_scaled : *state.gpu;

	auto *fb = reinterpret_cast<uint32_t *>(gpu.get_rdram() + 4096);
	for (int y = 0; y < 240; y++)
		for (int x = 0; x < 200; x++)
			fb[200 * y + x] = x << 24;

	gpu.set_vi_register(VIRegister::Origin, 4096);
	gpu.set_vi_register(VIRegister::Width, 200);
	gpu.set_vi_register(VIRegister::VStart, make_vi_start_register(VI_V_OFFSET_NTSC + 20 * 2, VI_V_OFFSET_NTSC + 200 * 2));

	gpu.begin_vi_register_per_scanline();
	gpu.set_vi_register_for_scanline(VI_V_OFFSET_NTSC,
	                                 make_vi_start_register(VI_H_OFFSET_NTSC, VI_H_OFFSET_NTSC + 320),
	                                 make_vi_scale_register(256, 0));

	gpu.set_vi_register_for_scanline(VI_V_OFFSET_NTSC + 50 * 2,
	                                 make_vi_start_register(VI_H_OFFSET_NTSC, VI_H_OFFSET_NTSC + 640),
	                                 make_vi_scale_register(240, 200));

	gpu.set_vi_register_for_scanline(VI_V_OFFSET_NTSC + 100 * 2,
	                                 make_vi_start_register(VI_H_OFFSET_NTSC - 8, VI_H_OFFSET_NTSC + 648),
	                                 make_vi_scale_register(220, 400));

	gpu.set_vi_register_for_scanline(VI_V_OFFSET_NTSC + 150 * 2,
	                                 make_vi_start_register(VI_H_OFFSET_NTSC + 8, VI_H_OFFSET_NTSC + 648),
	                                 make_vi_scale_register(210, 600));

	gpu.end_vi_register_per_scanline();

	std::vector<Interface::RGBA> reference_result;
	int scale_factor = upscale ? 2 : 1;
	int ref_width = 640 * scale_factor;
	int ref_height = 240 * scale_factor;
	reference_result.resize(ref_width * ref_height);

	const auto set_region = [&](int line, int h_start, int h_end, bool left_clamp, bool right_clamp, int x_add, int x_start) {
		int x_base = h_start;
		if (!left_clamp)
			h_start += 8;
		if (!right_clamp)
			h_end -= 7;

		Interface::RGBA *ptr = reference_result.data() + ref_width * line;

		int x_begin = std::max(h_start, 0);
		int x_end = std::min(h_end, 640);

		x_base *= scale_factor;
		x_begin *= scale_factor;
		x_end *= scale_factor;

		for (int x = x_begin; x < x_end; x++)
		{
			int sample_x = (x - x_base) * x_add + x_start * scale_factor;
			int x_frac = (sample_x >> 5) & 31;
			int x_lo = sample_x >> 10;
			int x_hi = x_lo + 1;

			x_lo /= scale_factor;
			x_hi /= scale_factor;

			int x_rounded = (x_lo * (32 - x_frac) + x_hi * x_frac + 16) >> 5;
			ptr[x].r = x_rounded;
		}
	};

	// Top 20 lines are blanked.
	for (int y = 20 * scale_factor; y < 50 * scale_factor; y++)
		set_region(y, 0, 320, false, false, 256, 0);
	for (int y = 50 * scale_factor; y < 100 * scale_factor; y++)
		set_region(y, 0, 640, false, false, 240, 200);
	for (int y = 100 * scale_factor; y < 150 * scale_factor; y++)
		set_region(y, -8, 648, true, true, 220, 400);
	for (int y = 150 * scale_factor; y < 200 * scale_factor; y++)
		set_region(y, 8, 640, false, true, 210, 600);

	if (crop)
	{
		gpu.set_crop_rect(9, 10, 11, 12);
		crop_image(reference_result, ref_width, ref_height,
		           9 * scale_factor, 10 * scale_factor, 11 * scale_factor, 12 * scale_factor);
	}

	if (args.capture)
		state.device->begin_renderdoc_capture();
	state.iface.set_context_index(0);
	gpu.end_frame();
	if (args.capture)
		state.device->end_renderdoc_capture();

	if (!compare_image(reference_result, ref_width, ref_height,
	                   state.iface.scanout_result[0], state.iface.widths[0], state.iface.heights[0]))
		return false;

	return true;
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

	suites.push_back({ "aa-none-rgba5551", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT;
		variant.fmt = VI_CONTROL_TYPE_RGBA5551_BIT;
		return run_conformance_vi(state, args, variant);
	}});
	suites.push_back({ "aa-none-rgba8888", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT;
		variant.fmt = VI_CONTROL_TYPE_RGBA8888_BIT;
		return run_conformance_vi(state, args, variant);
	}});
	suites.push_back({ "aa-none-blank", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT;
		variant.fmt = VI_CONTROL_TYPE_BLANK_BIT;
		return run_conformance_vi(state, args, variant);
	}});
	suites.push_back({ "aa-none-reserved", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT;
		variant.fmt = VI_CONTROL_TYPE_RESERVED_BIT;
		return run_conformance_vi(state, args, variant);
	}});

	suites.push_back({ "aa-extra-always", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_EXTRA_ALWAYS_BIT;
		variant.x_scale = 1198;
		variant.y_scale = 1234;
		return run_conformance_vi(state, args, variant);
	}});
	suites.push_back({ "aa-extra", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_EXTRA_BIT;
		variant.x_scale = 1198;
		variant.y_scale = 1234;
		return run_conformance_vi(state, args, variant);
	}});
	suites.push_back({ "aa-scale", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_ONLY_BIT;
		variant.x_scale = 1198;
		variant.y_scale = 1234;
		return run_conformance_vi(state, args, variant);
	}});
	suites.push_back({ "aa-none", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT;
		variant.x_scale = 1198;
		variant.y_scale = 1234;
		return run_conformance_vi(state, args, variant);
	}});

#define BOOL_OPTION_TEST(name, dither_en, divot_en, gamma_en, gamma_dith) \
	suites.push_back({ "aa-extra-" #name, [](ReplayerState &state, const Arguments &args) -> bool { \
		VITestVariant variant = {}; \
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_EXTRA_BIT; \
		variant.randomize_scale_bias = true; \
		variant.dither_filter = dither_en; \
		variant.divot = divot_en; \
		variant.gamma = gamma_en; \
		variant.gamma_dither = gamma_dith; \
		return run_conformance_vi(state, args, variant); \
	}})
	BOOL_OPTION_TEST(dither-filter, true, false, false, false);
	BOOL_OPTION_TEST(divot, false, true, false, false);
	BOOL_OPTION_TEST(dither-filter-divot, true, true, false, false);
	BOOL_OPTION_TEST(gamma, false, false, true, false);
	BOOL_OPTION_TEST(gamma-dither, false, false, true, true);
	BOOL_OPTION_TEST(nogamma-dither, false, false, false, true);

	suites.push_back({ "aa-none-randomize-xy-scale-bias", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.randomize_scale_bias = true;
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT;
		return run_conformance_vi(state, args, variant);
	}});
	suites.push_back({ "aa-scale-randomize-xy-scale-bias", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.randomize_scale_bias = true;
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_ONLY_BIT;
		return run_conformance_vi(state, args, variant);
	}});
	suites.push_back({ "aa-extra-randomize-xy-scale-bias", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.randomize_scale_bias = true;
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_EXTRA_BIT;
		return run_conformance_vi(state, args, variant);
	}});

	suites.push_back({ "aa-none-randomize-hv-start-end", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.randomize_start = true;
		variant.randomize_scale_bias = true;
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_ONLY_BIT;
		return run_conformance_vi(state, args, variant);
	}});

	suites.push_back({ "aa-none-randomize-hv-start-end-pal", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.randomize_start = true;
		variant.randomize_scale_bias = true;
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_ONLY_BIT;
		variant.pal = true;
		return run_conformance_vi(state, args, variant);
	}});

	suites.push_back({ "aa-none-serrate", [](ReplayerState &state, const Arguments &args) -> bool {
		VITestVariant variant = {};
		variant.aa = VI_CONTROL_AA_MODE_RESAMP_ONLY_BIT;
		variant.serrate = true;
		return run_conformance_vi(state, args, variant);
	}});

	suites.push_back({ "per-scanline-xh", [](ReplayerState &state, const Arguments &args) -> bool {
		return run_per_scanline_xh_vi(state, args, false, false);
	}});

	suites.push_back({ "per-scanline-xh-upscale", [](ReplayerState &state, const Arguments &args) -> bool {
		return run_per_scanline_xh_vi(state, args, true, false);
	}});

	suites.push_back({ "per-scanline-xh-crop", [](ReplayerState &state, const Arguments &args) -> bool {
		return run_per_scanline_xh_vi(state, args, false, true);
	}});

	suites.push_back({ "per-scanline-xh-upscale-crop", [](ReplayerState &state, const Arguments &args) -> bool {
		return run_per_scanline_xh_vi(state, args, true, true);
	}});

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
