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
#include "rdp_dump.hpp"
#include "cli_parser.hpp"
#include "context.hpp"
#include "device.hpp"

using namespace RDP;

static void print_help()
{
	LOGE("Usage: rdp-validate-dump\n"
	     "\t<Path to dump>\n"
	     "\t[--begin-frame <frame>]\n"
	     "\t[--sync-only]\n"
	);
}

static int main_inner(int argc, char *argv[])
{
	std::string path;
	unsigned begin_frame = 0;
	bool sync_only = false;
	bool capture = false;

	Util::CLICallbacks cbs;
	cbs.add("--help", [](Util::CLIParser &parser) { print_help(); parser.end(); });
	cbs.add("--begin-frame", [&](Util::CLIParser &parser) { begin_frame = parser.next_uint(); });
	cbs.add("--sync-only", [&](Util::CLIParser &) { sync_only = true; });
	cbs.add("--capture", [&](Util::CLIParser &) { capture = true; });
	cbs.default_handler = [&](const char *arg) { path = arg; };
	Util::CLIParser parser(std::move(cbs), argc - 1, argv + 1);

	if (capture)
		if (!Vulkan::Device::init_renderdoc_capture())
			LOGE("Failed to initialize RenderDoc capture.\n");

	if (!parser.parse())
	{
		print_help();
		return EXIT_FAILURE;
	}
	else if (parser.is_ended_state())
		return EXIT_SUCCESS;

	DumpPlayer player;
	if (!player.load_dump(path.c_str()))
	{
		LOGE("Failed to load dump: %s\n", path.c_str());
		return EXIT_FAILURE;
	}

	ReplayerState state;
	if (!state.init(player))
	{
		LOGE("Failed to initialize Vulkan device.\n");
		return EXIT_FAILURE;
	}

	auto &iface = state.iface;

	while (!state.iface.is_eof)
	{
		if (capture)
			state.device.begin_renderdoc_capture();

		unsigned current_draw_count = iface.draw_calls_for_context[1];
		unsigned current_frame_count = iface.frame_count_for_context[1];
		unsigned current_syncs = iface.syncs_for_context[1];
		while (current_frame_count == iface.frame_count_for_context[1] &&
		       ((!sync_only && current_draw_count == iface.draw_calls_for_context[1]) ||
		        (sync_only && current_syncs == iface.syncs_for_context[1])) &&
		       player.iterate())
		{
		}

		if (capture)
			state.device.end_renderdoc_capture();

		uint32_t fault_addr;
		bool fault_hidden;

		current_draw_count = iface.draw_calls_for_context[1];
		current_frame_count = iface.frame_count_for_context[1];
		current_syncs = iface.syncs_for_context[1];

		if (current_frame_count >= begin_frame &&
		    !compare_memory("TMEM", state.reference->get_tmem(), state.gpu->get_tmem(), 4096, &fault_addr))
		{
			if (sync_only)
			{
				LOGE("Dump validation failed in frame %u, sync %u!\n",
				     current_frame_count, current_syncs);
			}
			else
			{
				LOGE("Dump validation failed in frame %u, draw %u!\n",
				     current_frame_count, current_draw_count);
			}
			return EXIT_FAILURE;
		}

		if (current_frame_count >= begin_frame &&
		    !compare_rdram(*state.reference, *state.gpu, &fault_addr, &fault_hidden))
		{
			if (sync_only)
			{
				LOGE("Dump validation failed in frame %u, sync %u!\n",
				     current_frame_count, current_syncs);
			}
			else
			{
				LOGE("Dump validation failed in frame %u, draw %u!\n",
				     current_frame_count, current_draw_count);
			}

			if (fault_hidden)
				fault_addr *= 2;

			if (state.iface.fb.width)
			{
				int color_x, color_y, depth_x, depth_y;
				int color_offset = int(fault_addr - state.iface.fb.addr);
				int depth_offset = int(fault_addr - state.iface.fb.depth_addr);
				depth_offset >>= 1;

				switch (state.iface.fb.size)
				{
				case 2:
					color_offset >>= 1;
					break;

				case 3:
					color_offset >>= 2;
					break;

				default:
					break;
				}

				color_x = color_offset % state.iface.fb.width;
				color_y = color_offset / state.iface.fb.width;
				depth_x = depth_offset % state.iface.fb.width;
				depth_y = depth_offset / state.iface.fb.width;

				if ((color_offset <= depth_offset || depth_offset < 0) && color_offset >= 0)
				{
					if (fault_hidden)
						LOGE("Failure at hidden color coord (%d, %d).\n", color_x, color_y);
					else
						LOGE("Failure at color coord (%d, %d).\n", color_x, color_y);
				}
				else if ((depth_offset <= color_offset || color_offset < 0) && depth_offset >= 0)
				{
					if (fault_hidden)
						LOGE("Failure at hidden depth coord (%d, %d).\n", depth_x, depth_y);
					else
						LOGE("Failure at depth coord (%d, %d).\n", depth_x, depth_y);
				}
				else
					LOGE("Uncertain failure coordinate.\n");
			}

			return EXIT_FAILURE;
		}

		state.device.next_frame_context();

		if (current_frame_count >= begin_frame)
		{
			if (sync_only)
				LOGI("Passed frame %u, sync %u.\n", current_frame_count, current_syncs);
			else
				LOGI("Passed frame %u, draw %u.\n", current_frame_count, current_draw_count);
		}
	}

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	Granite::Global::init();
	int ret = main_inner(argc, argv);
	Granite::Global::deinit();
	return ret;
}