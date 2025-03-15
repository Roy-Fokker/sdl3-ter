module;

export module application;

import std;

// not dependent on SDL
import clock;
import io;
import camera;

// SDL specific
import types;
import pipeline;

using namespace std::literals;

export namespace ter
{
	class application
	{
	public:
		application(const application &)                     = default; // defaulted copy c'tor
		auto operator=(const application &) -> application & = default; // defaulted copy c'tor
		application(application &&)                          = default; // defaulted move c'tor
		auto operator=(application &&) -> application &      = default; // defaulted move c'tor

		application()
		{
			auto result = SDL_Init(SDL_INIT_VIDEO);
			assert(result and "SDL could not initialize.");

			make_window();
			make_gpu();

			result = SDL_ClaimWindowForGPUDevice(gpu.get(), wnd.get());
			assert(result and "Could not claim window for gpu");

			initialize_scene();
		}

		~application()
		{
			scn = {};

			SDL_ReleaseWindowFromGPUDevice(gpu.get(), wnd.get());

			wnd = {};
			gpu = {};
			SDL_Quit();
		}

		auto run() -> int
		{
			clk.reset();

			while (not quit)
			{
				handle_sdl_events();

				draw();

				clk.tick();
			}

			std::println("Elapsed Time: {}s", clk.get_elapsed<clock::s>());
			return 0;
		}

	private:
		void make_window();
		void make_gpu();

		void handle_sdl_events();
		void process_inputs(const SDL_Event &evt);

		void initialize_scene();
		void make_perspective();
		void place_camera();
		void make_gfx_pipeline();
		void make_comp_pipeline();
		void make_mesh();
		void load_texture();

		void draw();

	private:
		struct scene
		{
			SDL_FColor clear_color;
			struct uniform_data
			{
				glm::mat4 projection;
				glm::mat4 view;
			} proj_view;

			gfx_pipeline_ptr gfx_pipeline;
			gpu_buffer_ptr vertex_buffer;
			gpu_buffer_ptr index_buffer;
			uint32_t vertex_count;
			uint32_t index_count;

			comp_pipeline_ptr comp_pipeline;

			gpu_texture_ptr terrain_heightmap;
			gfx_sampler_ptr terrain_sampler;
		};

	private:
		ter::clock clk{};

		window_ptr wnd = nullptr;
		gpu_ptr gpu    = nullptr;

		SDL_Event sdl_event{};
		bool quit = false;

		scene scn{};

		camera cam{};
	};
}

using namespace ter;

namespace
{
	constexpr auto IS_DEBUG = bool{ _DEBUG };

	constexpr auto WND_WIDTH  = 1280;
	constexpr auto WND_HEIGHT = WND_WIDTH * 9 / 16;

	constexpr auto FOV_ANGLE = 90.f;
	constexpr auto FAR_PLANE = 100.f;

	constexpr auto TER_WIDTH = 1024;

	constexpr auto MAX_ANISOTROPY = float{ 16 };

	struct vertex
	{
		glm::vec3 pos = {};
		glm::vec2 uv  = {};
	};

	enum class sampler_type : uint8_t
	{
		point_clamp,
		point_wrap,
		linear_clamp,
		linear_wrap,
		anisotropic_clamp,
		anisotropic_wrap,
	};

	auto get_swapchain_texture(SDL_Window *win, SDL_GPUCommandBuffer *cmd_buf) -> SDL_GPUTexture *
	{
		auto sc_tex = (SDL_GPUTexture *)nullptr;

		auto res = SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, win, &sc_tex, NULL, NULL);
		assert(res == true and "Wait and acquire GPU swapchain texture failed.");
		assert(sc_tex != nullptr and "Swapchain texture is null. Is window minimized?");

		return sc_tex;
	}

	auto make_gpu_buffer(SDL_GPUDevice *gpu, SDL_GPUBufferUsageFlags usage, uint32_t size, std::string_view debug_name) -> gpu_buffer_ptr
	{
		auto buffer_info = SDL_GPUBufferCreateInfo{
			.usage = usage,
			.size  = size,
		};

		auto buffer = SDL_CreateGPUBuffer(gpu, &buffer_info);
		assert(buffer != nullptr and "Failed to create gpu buffer");

		if (IS_DEBUG and debug_name.size() > 0)
		{
			SDL_SetGPUBufferName(gpu, buffer, debug_name.data());
		}

		return { buffer, { gpu } };
	}

	auto make_gpu_texture(SDL_GPUDevice *gpu, ter::image_t::header_t img_hdr, std::string_view debug_name) -> gpu_texture_ptr
	{
		auto texture_info = SDL_GPUTextureCreateInfo{
			.type                 = SDL_GPU_TEXTURETYPE_2D,
			.format               = to_sdl(img_hdr.format),
			.usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER,
			.width                = img_hdr.width,
			.height               = img_hdr.height,
			.layer_count_or_depth = img_hdr.depth,
			.num_levels           = img_hdr.mipmap_count,
			.sample_count         = SDL_GPU_SAMPLECOUNT_1,
		};

		auto texture = SDL_CreateGPUTexture(gpu, &texture_info);
		assert(texture != nullptr and "Failed to create gpu texture.");

		if (IS_DEBUG and debug_name.size() > 0)
		{
			SDL_SetGPUTextureName(gpu, texture, debug_name.data());
		}

		return { texture, { gpu } };
	}

	auto make_gpu_sampler(SDL_GPUDevice *gpu, sampler_type type) -> gfx_sampler_ptr
	{
		auto sampler_info = [&]() -> SDL_GPUSamplerCreateInfo {
			switch (type)
			{
			case sampler_type::point_clamp:
				return {
					.min_filter        = SDL_GPU_FILTER_NEAREST,
					.mag_filter        = SDL_GPU_FILTER_NEAREST,
					.mipmap_mode       = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
					.address_mode_u    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
					.address_mode_v    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
					.address_mode_w    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
					.max_anisotropy    = 0,
					.enable_anisotropy = false,
				};
			case sampler_type::point_wrap:
				return {
					.min_filter        = SDL_GPU_FILTER_NEAREST,
					.mag_filter        = SDL_GPU_FILTER_NEAREST,
					.mipmap_mode       = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
					.address_mode_u    = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
					.address_mode_v    = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
					.address_mode_w    = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
					.max_anisotropy    = 0,
					.enable_anisotropy = false,
				};
			case sampler_type::linear_clamp:
				return {
					.min_filter        = SDL_GPU_FILTER_LINEAR,
					.mag_filter        = SDL_GPU_FILTER_LINEAR,
					.mipmap_mode       = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
					.address_mode_u    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
					.address_mode_v    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
					.address_mode_w    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
					.max_anisotropy    = 0,
					.enable_anisotropy = false,
				};
			case sampler_type::linear_wrap:
				return {
					.min_filter        = SDL_GPU_FILTER_LINEAR,
					.mag_filter        = SDL_GPU_FILTER_LINEAR,
					.mipmap_mode       = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
					.address_mode_u    = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
					.address_mode_v    = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
					.address_mode_w    = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
					.max_anisotropy    = 0,
					.enable_anisotropy = false,
				};
			case sampler_type::anisotropic_clamp:
				return {
					.min_filter        = SDL_GPU_FILTER_LINEAR,
					.mag_filter        = SDL_GPU_FILTER_LINEAR,
					.mipmap_mode       = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
					.address_mode_u    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
					.address_mode_v    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
					.address_mode_w    = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
					.max_anisotropy    = MAX_ANISOTROPY,
					.enable_anisotropy = true,
				};
			case sampler_type::anisotropic_wrap:
				return {
					.min_filter        = SDL_GPU_FILTER_LINEAR,
					.mag_filter        = SDL_GPU_FILTER_LINEAR,
					.mipmap_mode       = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
					.address_mode_u    = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
					.address_mode_v    = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
					.address_mode_w    = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
					.max_anisotropy    = MAX_ANISOTROPY,
					.enable_anisotropy = true,
				};
			}

			return {};
		}();

		auto sampler = SDL_CreateGPUSampler(gpu, &sampler_info);
		assert(sampler != nullptr and "Failed to create sampler.");

		return { sampler, { gpu } };
	}

	void upload_to_gpu(SDL_GPUDevice *gpu, SDL_GPUBuffer *buffer, byte_span src_data)
	{
		auto src_size = static_cast<uint32_t>(src_data.size());

		auto transfer_info = SDL_GPUTransferBufferCreateInfo{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size  = src_size,
		};
		auto transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &transfer_info);
		assert(transfer_buffer != nullptr and "Failed to create transfer buffer.");

		auto dst_data = SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false);
		std::memcpy(dst_data, src_data.data(), src_size);
		SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

		auto copy_cmd = SDL_AcquireGPUCommandBuffer(gpu);
		{
			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd);
			{
				auto src = SDL_GPUTransferBufferLocation{
					.transfer_buffer = transfer_buffer,
					.offset          = 0,
				};

				auto dst = SDL_GPUBufferRegion{
					.buffer = buffer,
					.offset = 0,
					.size   = src_size,
				};

				SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
			}
			SDL_EndGPUCopyPass(copy_pass);
			SDL_SubmitGPUCommandBuffer(copy_cmd);
		}
		SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
	}

	void upload_to_gpu(SDL_GPUDevice *gpu, SDL_GPUTexture *texture, const image_t &img)
	{
		auto src_size = static_cast<uint32_t>(img.data.size());

		auto transfer_info = SDL_GPUTransferBufferCreateInfo{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size  = src_size,
		};
		auto transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &transfer_info);
		assert(transfer_buffer != nullptr and "Failed to create transfer buffer.");

		auto dst_data = SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false);
		std::memcpy(dst_data, img.data.data(), src_size);
		SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

		auto copy_cmd = SDL_AcquireGPUCommandBuffer(gpu);
		{
			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd);
			{
				auto src = SDL_GPUTextureTransferInfo{
					.transfer_buffer = transfer_buffer,
					.offset          = 0,
				};

				auto dst = SDL_GPUTextureRegion{
					.texture = texture,
				};

				for (auto &&sub_image : img.sub_images)
				{
					src.offset = static_cast<uint32_t>(sub_image.offset);

					dst.mip_level = sub_image.mipmap_index;
					dst.layer     = sub_image.layer_index;
					dst.w         = sub_image.width;
					dst.h         = sub_image.height;
					dst.d         = 1;

					SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
				}
			}
			SDL_EndGPUCopyPass(copy_pass);
			SDL_SubmitGPUCommandBuffer(copy_cmd);
		}
		SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
	}
}

void application::make_window()
{
	constexpr auto title = "SDL3 Terrain"sv;

	auto window = SDL_CreateWindow(title.data(), WND_WIDTH, WND_HEIGHT, NULL);
	assert(window != nullptr and "Window could not be created.");

	// enable relative mouse movement
	SDL_SetWindowRelativeMouseMode(window, true);

	wnd = window_ptr{ window };
}

void application::make_gpu()
{
	auto gdev = SDL_CreateGPUDevice(SHADER_FORMAT, IS_DEBUG, NULL);
	assert(gdev != nullptr and "GPU device could not be created.");

	std::println("GPU API Name: {}", SDL_GetGPUDeviceDriver(gdev));

	gpu = gpu_ptr{ gdev };
}

void application::handle_sdl_events()
{
	while (SDL_PollEvent(&sdl_event))
	{
		switch (sdl_event.type)
		{
		case SDL_EVENT_QUIT:
			quit = true;
			return;
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_MOUSE_MOTION:
		case SDL_EVENT_MOUSE_WHEEL:
			process_inputs(sdl_event);
			break;
		}
	}
}

void application::process_inputs(const SDL_Event &evt)
{
	auto dt               = static_cast<float>(clk.get_delta<clock::s>());
	const auto move_speed = 5.f * dt;
	const auto rot_speed  = glm::radians(10.0f) * dt;

	auto handle_keyboard = [&](const SDL_KeyboardEvent &evt) {
		auto cam_dir = glm::vec3{};

		switch (evt.scancode)
		{
		case SDL_SCANCODE_ESCAPE:
			quit = true;
			break;
		case SDL_SCANCODE_W:
			cam_dir.z = 1.f;
			break;
		case SDL_SCANCODE_S:
			cam_dir.z = -1.f;
			break;
		case SDL_SCANCODE_A:
			cam_dir.x = -1.f;
			break;
		case SDL_SCANCODE_D:
			cam_dir.x = 1.f;
			break;
		case SDL_SCANCODE_Q:
			cam_dir.y = -1.f;
			break;
		case SDL_SCANCODE_E:
			cam_dir.y = 1.f;
			break;
		}
		cam.translate(cam_dir * move_speed);
	};

	auto handle_mouse = [&](const SDL_MouseMotionEvent &evt) {
		auto cam_rot = glm::vec3{};
		cam_rot.y    = -evt.xrel;
		cam_rot.x    = -evt.yrel;
		cam.rotate(cam_rot * rot_speed);
	};

	switch (evt.type)
	{
	case SDL_EVENT_KEY_DOWN:
		handle_keyboard(evt.key);
		break;
	case SDL_EVENT_MOUSE_MOTION:
		handle_mouse(evt.motion);
		break;
	}
}

void application::initialize_scene()
{
	scn.clear_color = { 0.2f, 0.4f, 0.4f, 1.0f };

	make_perspective();
	place_camera();

	make_gfx_pipeline();
	make_comp_pipeline();

	make_mesh();

	load_texture();
}

void application::make_perspective()
{
	float fovy         = glm::radians(FOV_ANGLE);
	float aspect_ratio = static_cast<float>(WND_WIDTH) / WND_HEIGHT;
	float near_plane   = 0.1f;

	scn.proj_view.projection = glm::perspective(fovy, aspect_ratio, near_plane, FAR_PLANE);
}

void application::place_camera()
{
	cam.lookat(glm::vec3{ 0.0f, 1.0f, -1.0f },
	           glm::vec3{ 0.0f, 0.0f, 0.0f },
	           glm::vec3{ 0.0f, 1.0f, 0.0f });
	scn.proj_view.view = cam.get_view();
}

void application::make_gfx_pipeline()
{
	auto vs_shdr = shader_builder{
		.shader_binary        = read_file("shaders/terrain.vs_6_4.cso"),
		.stage                = shader_stage::vertex,
		.uniform_buffer_count = 1,
	};

	auto fs_shdr = shader_builder{
		.shader_binary = read_file("shaders/terrain.ps_6_4.cso"),
		.stage         = shader_stage::fragment,
	};

	using VA = SDL_GPUVertexAttribute;
	auto va  = std::array{
        VA{
		   .location    = 0,
		   .buffer_slot = 0,
		   .format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
		   .offset      = 0,
        },
        VA{
		   .location    = 1,
		   .buffer_slot = 0,
		   .format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
		   .offset      = sizeof(glm::vec3),
        },
	};

	using VBD = SDL_GPUVertexBufferDescription;
	auto vbd  = std::array{
        VBD{
		   .slot       = 0,
		   .pitch      = sizeof(vertex),
		   .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        },
	};

	auto pl = gfx_pipeline_builder{
		.vertex_shader              = vs_shdr.build(gpu.get()),
		.fragment_shader            = fs_shdr.build(gpu.get()),
		.vertex_attributes          = va,
		.vertex_buffer_descriptions = vbd,
		.color_format               = SDL_GetGPUSwapchainTextureFormat(gpu.get(), wnd.get()),
		.enable_depth_test          = false,
		.culling                    = cull_mode::none,
	};
	scn.gfx_pipeline = pl.build(gpu.get());
}

void application::make_comp_pipeline()
{
	auto pl = comp_pipeline_builder{
		.shader_binary                   = read_file("shaders/terrain.cs_6_4.cso"),
		.sampler_count                   = 1,
		.readwrite_storage_uniform_count = 1,
		.thread_count                    = { 1024, 1024, 1 },
	};

	scn.comp_pipeline = pl.build(gpu.get());
}

void application::make_mesh()
{
	auto vertices = std::vector<vertex>{};
	auto indices  = std::vector<uint32_t>{};

	{ // Make Mesh
		auto x = 0.5f, y = 0.0f, z = 0.5f;
		auto w_2 = TER_WIDTH / 2;

		namespace vw = std::views;
		namespace rg = std::ranges;

		const auto vtx_square = std::array{
			vertex{ { -x, y, -z }, { 0.f, 1.f } },
			vertex{ { +x, y, -z }, { 1.f, 1.f } },
			vertex{ { +x, y, +z }, { 1.f, 0.f } },
			vertex{ { -x, y, +z }, { 0.f, 0.f } },
		};
		const auto idx_square = std::array<uint32_t, 6>{
			0, 1, 2, //
			2, 3, 0, //
		};
		for (auto i : vw::iota(-w_2, w_2))
		{
			for (auto j : vw::iota(-w_2, w_2))
			{
				auto v_offset = glm::vec3{ i, 0, j };
				rg::transform(vtx_square, std::back_inserter(vertices), [&](const auto &vtx) {
					return vertex{
						vtx.pos + v_offset,
						vtx.uv,
					};
				});

				auto i_offset = vertices.size() - 4;
				rg::transform(idx_square, std::back_inserter(indices), [&](const auto &idx) {
					return static_cast<uint32_t>(idx + i_offset);
				});
			}
		}
	}

	{ // Create Buffer on GPU and populate
		scn.vertex_count = static_cast<uint32_t>(vertices.size());
		scn.index_count  = static_cast<uint32_t>(indices.size());

		auto vb_size      = static_cast<uint32_t>(scn.vertex_count * sizeof(vertex));
		scn.vertex_buffer = make_gpu_buffer(gpu.get(), SDL_GPU_BUFFERUSAGE_VERTEX, vb_size, "Terrain Vertices");
		upload_to_gpu(gpu.get(), scn.vertex_buffer.get(), as_byte_span(vertices));

		auto ib_size     = static_cast<uint32_t>(scn.index_count * sizeof(uint32_t));
		scn.index_buffer = make_gpu_buffer(gpu.get(), SDL_GPU_BUFFERUSAGE_INDEX, ib_size, "Terrain Indices");
		upload_to_gpu(gpu.get(), scn.index_buffer.get(), as_byte_span(indices));
	}
}

void application::load_texture()
{
	auto ter_height = read_ddsktx_file("data/Mount_Fuji.dds");

	scn.terrain_heightmap = make_gpu_texture(gpu.get(), ter_height.header, "Terrain Heightmap");
	upload_to_gpu(gpu.get(), scn.terrain_heightmap.get(), ter_height);

	scn.terrain_sampler = make_gpu_sampler(gpu.get(), sampler_type::point_wrap);
}

void application::draw()
{
	scn.proj_view.view = cam.get_view();

	auto device = gpu.get();
	auto window = wnd.get();

	auto cmd_buf = SDL_AcquireGPUCommandBuffer(device);
	assert(cmd_buf != nullptr and "Failed to acquire command buffer.");

	// Push Uniform buffer
	auto uniform_data = as_byte_span(scn.proj_view);
	SDL_PushGPUVertexUniformData(cmd_buf, 0, uniform_data.data(), static_cast<uint32_t>(uniform_data.size()));

	auto sc_img = get_swapchain_texture(window, cmd_buf);

	auto color_target = SDL_GPUColorTargetInfo{
		.texture     = sc_img,
		.clear_color = scn.clear_color,
		.load_op     = SDL_GPU_LOADOP_CLEAR,
		.store_op    = SDL_GPU_STOREOP_STORE,
	};

	auto render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, nullptr);
	{
		SDL_BindGPUGraphicsPipeline(render_pass, scn.gfx_pipeline.get());

		auto vertex_bindings = std::array{
			SDL_GPUBufferBinding{
			  .buffer = scn.vertex_buffer.get(),
			  .offset = 0,
			},
		};
		SDL_BindGPUVertexBuffers(render_pass, 0, vertex_bindings.data(), static_cast<uint32_t>(vertex_bindings.size()));

		auto index_binding = SDL_GPUBufferBinding{
			.buffer = scn.index_buffer.get(),
			.offset = 0,
		};
		SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

		SDL_DrawGPUIndexedPrimitives(render_pass, scn.index_count, 1, 0, 0, 0);
	}
	SDL_EndGPURenderPass(render_pass);

	SDL_SubmitGPUCommandBuffer(cmd_buf);
}
