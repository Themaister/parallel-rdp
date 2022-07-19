/* Copyright (c) 2022 Themaister
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

#include "rdp_device.hpp"
#include "context.hpp"
#include "device.hpp"
#include "wsi.hpp"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "logging.hpp"
#include <stddef.h>
#include <cmath>

// The non-standalone parallel-rdp requires some filesystem access for shaders which we have to handle here.
// When using the standalone build, all shaders are precompiled and codepath that is ifdeffed here can be omitted.
#define PARALLEL_RDP_STANDALONE 0

#if !PARALLEL_RDP_STANDALONE
#include "global_managers_init.hpp"
#endif

// A demo showing how to blit parallel-rdp frames on screen using Granite with GL interop.

constexpr unsigned WIDTH = 1280;
constexpr unsigned HEIGHT = 720;

constexpr unsigned SCANOUT_ORIGIN = 1024;
constexpr unsigned SCANOUT_WIDTH = 320;
constexpr unsigned SCANOUT_HEIGHT = 240;

static void check_gl_error()
{
	GLenum err;
	if ((err = glGetError()) != GL_NO_ERROR)
	{
		LOGE("GL error: #%x.\n", err);
		exit(EXIT_FAILURE);
	}
}

static void import_semaphore(GLuint &glsem, const Vulkan::ExternalHandle &handle)
{
	glGenSemaphoresEXT(1, &glsem);
#ifdef _WIN32
	glImportSemaphoreWin32HandleEXT(glsem, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, handle.handle);
	CloseHandle(handle.handle);
#else
	// Importing an FD takes ownership of it.
	glImportSemaphoreFdEXT(glsem, GL_HANDLE_TYPE_OPAQUE_FD_EXT, handle.handle);
#endif
	check_gl_error();
}

static void setup_default_vi_registers(RDP::CommandProcessor &processor)
{
	processor.set_vi_register(RDP::VIRegister::Control,
	                          RDP::VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT |
	                          RDP::VI_CONTROL_TYPE_RGBA8888_BIT);
	processor.set_vi_register(RDP::VIRegister::Origin, SCANOUT_ORIGIN);
	processor.set_vi_register(RDP::VIRegister::Width, SCANOUT_WIDTH);
	processor.set_vi_register(RDP::VIRegister::VSync, RDP::VI_V_SYNC_NTSC);
	processor.set_vi_register(RDP::VIRegister::XScale,
	                          RDP::make_vi_scale_register(512, 0));
	processor.set_vi_register(RDP::VIRegister::YScale,
	                          RDP::make_vi_scale_register(1024, 0));
	processor.set_vi_register(RDP::VIRegister::VStart,
	                          RDP::make_vi_start_register(RDP::VI_V_OFFSET_NTSC,
	                                                      RDP::VI_V_OFFSET_NTSC + 224 * 2));
	processor.set_vi_register(RDP::VIRegister::HStart,
	                          RDP::make_vi_start_register(RDP::VI_H_OFFSET_NTSC,
	                                                      RDP::VI_H_OFFSET_NTSC + 640));
}

static void update_vram(void *vram_ptr, unsigned frame_index)
{
	auto *base = static_cast<uint32_t *>(vram_ptr) + SCANOUT_ORIGIN / 4;

	for (unsigned y = 0; y < SCANOUT_HEIGHT; y++)
	{
		for (unsigned x = 0; x < SCANOUT_WIDTH; x++)
		{
			// Generate a funky pattern.

			float r = std::sin(float(x) * 0.134f + float(y) * 0.234f + float(frame_index) * 0.05f);
			float g = std::sin(float(x) * 0.434f + float(y) * 0.234f + float(frame_index) * 0.02f);
			float b = std::sin(float(x) * -0.234f + float(y) * -0.234f + float(frame_index) * 0.03f);

			r = r * 0.4f + 0.5f;
			g = g * 0.4f + 0.5f;
			b = b * 0.4f + 0.5f;

			auto ur = unsigned(r * 255.0f);
			auto ug = unsigned(g * 255.0f);
			auto ub = unsigned(b * 255.0f);

			uint32_t color = (ur << 24) | (ug << 16) | (ub << 8);
			base[y * SCANOUT_WIDTH + x] = color;
		}
	}
}

static void render_frame(Vulkan::Device &device, RDP::CommandProcessor &processor, GLFWwindow *window)
{
	RDP::ScanoutOptions options = {};
	options.export_scanout = true;
	// WIN32 HANDLE or POSIX fd, pick the platform default.
	options.export_handle_type = Vulkan::ExternalHandle::get_opaque_memory_handle_type();
	Vulkan::ImageHandle image = processor.scanout(options);

	if (!image)
	{
		// Blank screen case.
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(window);
		device.next_frame_context();
		return;
	}

	auto exported_image = image->export_handle();

	// Import image as a texture.
	GLuint gltex;
	GLuint glmem;
	GLuint glfbo;
	glCreateTextures(GL_TEXTURE_2D, 1, &gltex);
	glCreateMemoryObjectsEXT(1, &glmem);
	glCreateFramebuffers(1, &glfbo);

	// We always use dedicated allocations in Granite for external objects.
	GLint gltrue = GL_TRUE;
	glMemoryObjectParameterivEXT(glmem, GL_DEDICATED_MEMORY_OBJECT_EXT, &gltrue);

#ifdef _WIN32
	glImportMemoryWin32HandleEXT(glmem, image->get_allocation().get_size(),
	                             GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, exported_image.handle);
#else
	// Importing takes ownership of the FD.
	glImportMemoryFdEXT(glmem, image->get_allocation().get_size(),
	                    GL_HANDLE_TYPE_OPAQUE_FD_EXT, exported_image.handle);
#endif

	check_gl_error();

	if (image->get_format() != VK_FORMAT_R8G8B8A8_UNORM)
	{
		LOGE("Unexpected format for scanout image.\n");
		exit(EXIT_FAILURE);
	}

	glTextureStorageMem2DEXT(gltex, 1, GL_RGBA8,
	                         GLsizei(image->get_width()),
	                         GLsizei(image->get_height()),
	                         glmem, 0);

	check_gl_error();

#ifdef _WIN32
	// The HANDLE seems to be consumed at TextureStorage time, otherwise we get OUT_OF_MEMORY error on NV Windows.
	// Sort of makes sense since it's a dedicated allocation?
	CloseHandle(exported_image.handle);
#endif

	// We'll blit the result to screen with BlitFramebuffer so we don't have to bother with shaders for this sample.
	// Normally you would bind it as a texture or whatever.
	glNamedFramebufferTexture(glfbo, GL_COLOR_ATTACHMENT0, gltex, 0);
	GLenum status;
	if ((status = glCheckNamedFramebufferStatus(glfbo, GL_READ_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
	{
		LOGE("Failed to bind framebuffer (#%x).\n", status);
		exit(EXIT_FAILURE);
	}

	// Vulkan -> GL sync.
	{
		// Exportable timeline is not widely supported sadly.
		auto signal_semaphore = device.request_semaphore_external(
				VK_SEMAPHORE_TYPE_BINARY_KHR,
				Vulkan::ExternalHandle::get_opaque_semaphore_handle_type());

		// scanout() already performed the barrier to EXTERNAL queue,
		// so just submit a signal to the external semaphore.
		device.submit_empty(Vulkan::CommandBuffer::Type::Generic, nullptr, signal_semaphore.get());
		auto exported_signal = signal_semaphore->export_to_handle();

		GLuint glsem;
		import_semaphore(glsem, exported_signal);

		// Wait. The layout matches whatever we used when releasing the image.
		GLenum gllayout = GL_LAYOUT_SHADER_READ_ONLY_EXT;
		glWaitSemaphoreEXT(glsem, 0, nullptr, 1, &gltex, &gllayout);
		glDeleteSemaphoresEXT(1, &glsem);
	}

	// GL work, just blit the frame to backbuffer to avoid dealing with shaders and all that.
	{
		int fb_width, fb_height;
		glfwGetFramebufferSize(window, &fb_width, &fb_height);

		// Y-flip here as well.
		glBlitNamedFramebuffer(glfbo, 0,
		                       0, GLint(image->get_height()), GLint(image->get_width()), 0,
		                       0, 0, fb_width, fb_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

	// GL -> Vulkan sync.
	// If we're using e.g. interlace, the frontend might reuse the image for field blending, so
	// we have to satisfy the WAR hazards.
	// Synchronize with OpenGL. Export a handle that GL can signal.
	// We could have reused the GL semaphore, but that causes weird issues on NV, so don't ...
	{
		auto wait_semaphore = device.request_semaphore_external(
				VK_SEMAPHORE_TYPE_BINARY_KHR, Vulkan::ExternalHandle::get_opaque_semaphore_handle_type());
		// Have to mark the semaphore is signalled since we assert on that being the case when exporting a semaphore.
		wait_semaphore->signal_external();
		auto exported_semaphore = wait_semaphore->export_to_handle();

		GLuint glsem;
		GLenum gllayout = GL_LAYOUT_SHADER_READ_ONLY_EXT;
		import_semaphore(glsem, exported_semaphore);
		glSignalSemaphoreEXT(glsem, 0, nullptr, 1, &gltex, &gllayout);

		// Unsure if we have to flush to make sure the signal has been processed.
		glFlush();

		// Add the write-after-read barrier.
		device.add_wait_semaphore(Vulkan::CommandBuffer::Type::Generic, std::move(wait_semaphore),
		                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, true);

		glDeleteSemaphoresEXT(1, &glsem);
	}

	glfwSwapBuffers(window);
	glDeleteFramebuffers(1, &glfbo);
	glDeleteTextures(1, &gltex);
	glDeleteMemoryObjectsEXT(1, &glmem);

	check_gl_error();

	// Important that we iterate the frame context to reclaim memory, wait for fences, etc.
	device.next_frame_context();
}

int main()
{
	if (!glfwInit())
	{
		LOGE("Failed to initialize GLFW.\n");
		return EXIT_FAILURE;
	}

	if (!Vulkan::Context::init_loader(nullptr))
	{
		LOGE("Failed to initialize Vulkan loader.\n");
		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

	auto *window = glfwCreateWindow(WIDTH, HEIGHT, "parallel-rdp-demo-gl",
	                                nullptr, nullptr);

	if (!window)
	{
		LOGE("Failed to create window.\n");
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		LOGE("Failed to load GL context functions.\n");
		return EXIT_FAILURE;
	}

	if (!GLAD_GL_EXT_memory_object || !GLAD_GL_EXT_semaphore)
	{
		LOGE("External functions not supported.\n");
		return EXIT_FAILURE;
	}

#ifdef _WIN32
	if (!GLAD_GL_EXT_memory_object_win32 || !GLAD_GL_EXT_semaphore_win32)
	{
		LOGE("External handle functions not supported.\n");
		return EXIT_FAILURE;
	}
#else
	if (!GLAD_GL_EXT_memory_object_fd || !GLAD_GL_EXT_semaphore_fd)
	{
		LOGE("External FD functions not supported.\n");
		return EXIT_FAILURE;
	}
#endif

	glfwShowWindow(window);
	glfwSwapInterval(1);

	Vulkan::Context::SystemHandles handles = {};
	// For Granite to have access to filesystem, threading, etc, we need to
	// pass down handles.
	// For this kind of integration where we only need to blit stuff,
	// we don't need any of this.
	// We don't have to build in lots of code for shader compilers, filesystems, etc ...

#if !PARALLEL_RDP_STANDALONE
	// However, this only applies to the standalone parallel-rdp build in
	// Themaister/parallel-rdp-standalone.
	// This repo is used for development, and does runtime shader compilation, so make sure we enable filesystem
	// support if we have to.
	Granite::Global::init(Granite::Global::MANAGER_FEATURE_FILESYSTEM_BIT);
	handles.filesystem = GRANITE_FILESYSTEM();
#endif

	Vulkan::Context context;
	context.set_system_handles(handles);
	if (!context.init_instance_and_device(nullptr, 0, nullptr, 0))
	{
		LOGE("Failed to create Vulkan device.\n");
		return EXIT_FAILURE;
	}

	Vulkan::Device device;
	device.set_context(context);

	if (!device.get_device_features().supports_external)
	{
		LOGE("Device does not support external sharing.\n");
		return EXIT_FAILURE;
	}

	// Validate that the GL device and Vulkan device match. Relevant for Win32 where LUID is a thing.
	auto &features = device.get_device_features();
	if (features.id_properties.deviceLUIDValid)
	{
		GLubyte luid[GL_LUID_SIZE_EXT] = {};
		glGetUnsignedBytevEXT(GL_DEVICE_LUID_EXT, luid);

		if (memcmp(features.id_properties.deviceLUID, luid, GL_LUID_SIZE_EXT) != 0)
		{
			LOGE("LUID mismatch.\n");
			return EXIT_FAILURE;
		}
	}

	constexpr unsigned RDRAM_SIZE = 4 * 1024 * 1024;

	// The imported base pointer must be sufficiently aligned to
	// support external host memory. 64 KiB is good enough.
	// rdram_offset can be used to point to an offset within rdram_ptr.
	void *rdram_ptr = Util::memalign_calloc(64 * 1024, RDRAM_SIZE);
	auto processor = std::make_unique<RDP::CommandProcessor>(
			device,
			rdram_ptr, 0 /* offset */,
			RDRAM_SIZE, RDRAM_SIZE / 8,
			RDP::CommandProcessorFlags{});

	if (!processor->device_is_supported())
	{
		LOGE("Vulkan device does not support required features for parallel-rdp.\n");
		return EXIT_FAILURE;
	}

	setup_default_vi_registers(*processor);
	unsigned frame_count = 0;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Update VRAM with something that animates for purposes of scanout.
		update_vram(rdram_ptr, frame_count++);

		// Scanout VI and blit the result to screen.
		render_frame(device, *processor, window);
	}

	processor.reset();

	glfwDestroyWindow(window);
	glfwTerminate();
	Util::memalign_free(rdram_ptr);
}
