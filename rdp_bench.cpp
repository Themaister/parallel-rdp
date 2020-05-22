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

#include "conformance_utils.hpp"
#include "global_managers.hpp"
#include "cli_parser.hpp"
#include "timer.hpp"
#include "application_cli_wrapper.hpp"
#include <stdlib.h>

using namespace RDP;

static InputPrimitive generate_input_primitive()
{
	InputPrimitive prim = {};
	for (auto &vert : prim.vertices)
	{
		vert.z = 0.5f;
		vert.w = 1.0f;
	}

	prim.vertices[0].x = -1.0f;
	prim.vertices[0].y = -1.0f;
	prim.vertices[1].x = -1.0f;
	prim.vertices[1].y = +3.0f;
	prim.vertices[2].x = +3.0f;
	prim.vertices[2].y = -1.0f;

	prim.vertices[0].u = 0.0f;
	prim.vertices[0].v = 0.0f;
	prim.vertices[1].u = 0.0f;
	prim.vertices[1].v = 500.0f;
	prim.vertices[2].u = 500.0f;
	prim.vertices[2].v = 0.0f;

	prim.vertices[0].color[0] = 1.0f;
	prim.vertices[1].color[1] = 1.0f;
	prim.vertices[2].color[2] = 1.0f;

	return prim;
}

static int main_inner(int, char **)
{
#ifdef _WIN32
	_putenv("PARALLEL_RDP_FORCE_SYNC_SHADER=1");
	_putenv("PARALLEL_RDP_SINGLE_THREADED_COMMAND=1");
	_putenv("PARALLEL_RDP_BENCH=1");
#else
	setenv("PARALLEL_RDP_FORCE_SYNC_SHADER", "1", 1);
	setenv("PARALLEL_RDP_SINGLE_THREADED_COMMAND", "1", 1);
	setenv("PARALLEL_RDP_BENCH", "1", 1);
#endif

	ReplayerState state;
	if (!state.init())
		return EXIT_FAILURE;

	const unsigned iterations = 10000;
	const unsigned num_quads_per_frame = 10;
	const unsigned width = 512;
	const unsigned height = 256;

	auto prim = generate_input_primitive();

	state.builder.set_command_interface(state.gpu.get());
	state.builder.set_viewport({ 0, 0, width, height, 0, 1 });
	state.builder.set_color_image(TextureFormat::RGBA, TextureSize::Bpp16, 512, 512);
	state.builder.set_depth_image(2 * 1024 * 1024);

	state.builder.set_depth_write(true);
	state.builder.set_cycle_type(CycleType::Cycle2);
	state.builder.set_combiner_1cycle({{ RGBMulAdd::Shade, RGBMulSub::Texel0, RGBMul::LODFrac, RGBAdd::Zero },
	                                  { AlphaAddSub::ShadeAlpha, AlphaAddSub::Zero, AlphaMul::Texel0Alpha, AlphaAddSub::Zero }});
	state.builder.set_scissor(0, 0, width, height);

	TileMeta meta = {};
	meta.size = TextureSize::Bpp16;
	meta.fmt = TextureFormat::RGBA;
	meta.stride = 32;
	meta.flags = TILE_INFO_CLAMP_S_BIT | TILE_INFO_CLAMP_T_BIT;
	state.builder.set_tile(0, meta);
	state.builder.set_tile_size(0, 0, 0, 16, 16);
	meta.offset = 2048;
	state.builder.set_tile(1, meta);
	state.builder.set_tile_size(1, 0, 0, 16, 16);

	std::vector<uint64_t> timestamps(iterations);

	for (unsigned iter = 0; iter < iterations; iter++)
	{
		state.builder.set_color_image(TextureFormat::RGBA, TextureSize::Bpp16, (iter & 3) * 512, width);
		for (unsigned count = 0; count < num_quads_per_frame; count++)
			state.builder.draw_triangle(prim);
		state.device.next_frame_context();
		timestamps[iter] = Util::get_current_time_nsecs();

		if ((iter & 127) == 127)
			LOGI("...\n");
	}

	state.device.wait_idle();

	uint64_t delta_ns = timestamps[iterations - 3] - timestamps[3];
	double delta_s = 1e-9 * double(delta_ns);
	uint64_t num_frames = iterations - 6;
	uint64_t num_pixels = num_frames * num_quads_per_frame * width * height;
	double time_per_frame = (1e-9 * double(delta_ns)) / double(num_frames);

	LOGI("Time per frame: %.3f ms.\n", 1000.0 * time_per_frame);
	LOGI("Fill-rate: %.6f Gpixels/s.\n", 1e-9 * double(num_pixels) / delta_s);
	return EXIT_SUCCESS;
}

#ifdef WRAPPER_CLI
namespace Granite
{
Application *application_create(int argc, char **argv)
{
	application_dummy();
	setup_filesystems();
	return new ApplicationCLIWrapper(main_inner, argc, argv);
}
}
#else
int main(int argc, char **argv)
{
	Granite::Global::init();
	setup_filesystems();
	int ret = main_inner(argc, argv);
	Granite::Global::deinit();
	return ret;
}
#endif
