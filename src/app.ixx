module;

export module application;

import std;

// not dependent on SDL
import clock;
import io;
import camera;

// SDL wrapper
import sdl;

using namespace std::literals;
namespace vw = std::views;
namespace rg = std::ranges;

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
			make_window();
			make_gpu();

			initialize_scene();
		}

		~application() = default;

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

		void run_compute();
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

			gfx_pipeline_ptr ter_cs_gfx_pl; // using compute shader
			gpu_buffer_ptr ter_cs_vtx_buffer;
			gpu_buffer_ptr ter_cs_idx_buffer;
			uint32_t ter_cs_vtx_cnt;
			uint32_t ter_cs_idx_cnt;
			comp_pipeline_ptr comp_pipeline;

			gfx_pipeline_ptr ter_cm_gfx_pl; // not using compute shader
			gpu_buffer_ptr ter_cm_vtx_buffer;
			gpu_buffer_ptr ter_cm_idx_buffer;
			uint32_t ter_cm_vtx_cnt;
			uint32_t ter_cm_idx_cnt;

			gpu_texture_ptr depth_texture;
			gpu_texture_ptr terrain_heightmap;
			gfx_sampler_ptr terrain_sampler;
		};

	private:
		ter::clock clk{};

		sdl_base sdl = {};

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
	constexpr auto WND_WIDTH  = 1280;
	constexpr auto WND_HEIGHT = WND_WIDTH * 9 / 16;

	constexpr auto SHADER_FORMAT = SDL_GPU_SHADERFORMAT_DXIL;

	constexpr auto FOV_ANGLE = 60.f;
	constexpr auto FAR_PLANE = 1200.f;

	constexpr auto TER_WIDTH = 1024;

	struct vertex
	{
		glm::vec3 pos = {};
		glm::vec2 uv  = {};
	};
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

	auto result = SDL_ClaimWindowForGPUDevice(gdev, wnd.get());
	assert(result and "Could not claim window for gpu");

	gpu = gpu_ptr{ gdev, { wnd.get() } };
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
	const auto move_speed = 50.f * dt;
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

	// run_compute();
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
	cam.lookat(glm::vec3{ 510.0f, 1.0f, -510.0f },
	           glm::vec3{ 0.0f, 0.0f, 0.0f },
	           glm::vec3{ 0.0f, 1.0f, 0.0f });
	scn.proj_view.view = cam.get_view();
}

void application::make_gfx_pipeline()
{
	{ // Pipeline depending on Compute Shader modified vertex buffer
		auto vs_shdr = shader_builder{
			.shader_binary        = read_file("shaders/terrain.vs_6_4.cso"),
			.stage                = shader_stage::vertex,
			.uniform_buffer_count = 1,
		};

		auto fs_shdr = shader_builder{
			.shader_binary = read_file("shaders/terrain.ps_6_4.cso"),
			.stage         = shader_stage::fragment,
			.sampler_count = 1,
		};

		using VA = SDL_GPUVertexAttribute;
		auto va  = std::array{
            VA{
			   .location    = 0,
			   .buffer_slot = 0,
			   .format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			   .offset      = offsetof(vertex, pos),
            },
            VA{
			   .location    = 1,
			   .buffer_slot = 0,
			   .format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			   .offset      = offsetof(vertex, uv), // sizeof(glm::vec3),
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
			.enable_depth_stencil       = true,
			.raster                     = raster_type::back_ccw_fill,
			.blend                      = blend_type::none,
			.topology                   = topology_type::triangle_list,
		};
		scn.ter_cs_gfx_pl = pl.build(gpu.get());
	}

	{ // Clipmap pipeline
		auto vs_shdr = shader_builder{
			.shader_binary        = read_file("shaders/cm_terrain.vs_6_4.cso"),
			.stage                = shader_stage::vertex,
			.sampler_count        = 1,
			.uniform_buffer_count = 1,
		};

		auto fs_shdr = shader_builder{
			.shader_binary = read_file("shaders/cm_terrain.ps_6_4.cso"),
			.stage         = shader_stage::fragment,
			.sampler_count = 1,
		};

		using VA = SDL_GPUVertexAttribute;
		auto va  = std::array{
            VA{
			   .location    = 0,
			   .buffer_slot = 0,
			   .format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			   .offset      = offsetof(vertex, pos),
            },
            VA{
			   .location    = 1,
			   .buffer_slot = 0,
			   .format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			   .offset      = offsetof(vertex, uv), // sizeof(glm::vec3),
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
			.enable_depth_stencil       = true,
			.raster                     = raster_type::back_ccw_fill,
			.blend                      = blend_type::none,
			.topology                   = topology_type::triangle_list,
		};
		scn.ter_cm_gfx_pl = pl.build(gpu.get());
	}
}

void application::make_comp_pipeline()
{
	auto pl = comp_pipeline_builder{
		.shader_binary                   = read_file("shaders/terrain.cs_6_4.cso"),
		.sampler_count                   = 1,
		.readwrite_storage_uniform_count = 1,
		.thread_count                    = { 1024, 1, 1 },
	};

	scn.comp_pipeline = pl.build(gpu.get());
}

void application::make_mesh()
{
	auto vertices = std::vector<vertex>{};
	auto indices  = std::vector<uint32_t>{};

	{ // Make Mesh
		const auto x = 0.5f, y = 0.0f, z = 0.5f;
		const auto w_2 = TER_WIDTH / 2;

		const auto uv         = 1.f / TER_WIDTH;
		const auto vtx_square = std::array{
			vertex{ { -x, y, -z }, { 0.f, 0.f } },
			vertex{ { +x, y, -z }, { uv, 0.f } },
			vertex{ { +x, y, +z }, { uv, uv } },
			vertex{ { -x, y, +z }, { 0.f, uv } },
		};
		const auto idx_square = std::array<uint32_t, 6>{
			0, 1, 2, //
			2, 3, 0, //
		};
		auto rng = vw::iota(-w_2, w_2);
		for (auto [i, j] : vw::cartesian_product(rng, rng))
		{
			auto v_offset  = glm::vec3{ i, 0, j };
			auto uv_offset = glm::vec2{ uv * (i + w_2), uv * (j + w_2) };
			rg::transform(vtx_square, std::back_inserter(vertices), [&](const auto &vtx) {
				return vertex{
					vtx.pos + v_offset,
					vtx.uv + uv_offset,
				};
			});

			auto i_offset = vertices.size() - 4;
			rg::transform(idx_square, std::back_inserter(indices), [&](const auto &idx) {
				return static_cast<uint32_t>(idx + i_offset);
			});
		}
	}

	{ // Create Buffer on GPU and populate
		scn.ter_cs_vtx_cnt = static_cast<uint32_t>(vertices.size());
		scn.ter_cs_idx_cnt = static_cast<uint32_t>(indices.size());

		auto vb_size          = static_cast<uint32_t>(scn.ter_cs_vtx_cnt * sizeof(vertex));
		scn.ter_cs_vtx_buffer = make_gpu_buffer(gpu.get(), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_VERTEX, vb_size, "Terrain Vertices");
		upload_to_gpu(gpu.get(), scn.ter_cs_vtx_buffer.get(), as_byte_span(vertices));

		auto ib_size          = static_cast<uint32_t>(scn.ter_cs_idx_cnt * sizeof(uint32_t));
		scn.ter_cs_idx_buffer = make_gpu_buffer(gpu.get(), SDL_GPU_BUFFERUSAGE_INDEX, ib_size, "Terrain Indices");
		upload_to_gpu(gpu.get(), scn.ter_cs_idx_buffer.get(), as_byte_span(indices));
	}
}

void application::load_texture()
{
	auto ter_height = read_ddsktx_file("data/Mount_Fuji.dds");

	scn.terrain_heightmap = make_gpu_texture(gpu.get(), ter_height.header, "Terrain Heightmap");
	upload_to_gpu(gpu.get(), scn.terrain_heightmap.get(), ter_height);

	scn.terrain_sampler = make_gpu_sampler(gpu.get(), sampler_type::anisotropic_clamp);

	// Depth Texture
	auto w = 0, h = 0;
	SDL_GetWindowSizeInPixels(wnd.get(), &w, &h);

	auto texture_info = SDL_GPUTextureCreateInfo{
		.type                 = SDL_GPU_TEXTURETYPE_2D,
		.format               = get_gpu_supported_depth_stencil_format(gpu.get()),
		.usage                = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.width                = static_cast<uint32_t>(w),
		.height               = static_cast<uint32_t>(h),
		.layer_count_or_depth = 1,
		.num_levels           = 1,
		.sample_count         = SDL_GPU_SAMPLECOUNT_1,
	};
#ifdef WINDOWS
	if (texture_info.usage & SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)
	{
		auto props = SDL_CreateProperties();
		SDL_SetFloatProperty(props, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, 1.0f);
		texture_info.props = props;
	}
#endif
	scn.depth_texture = make_gpu_texture(gpu.get(), texture_info, "Depth Texture");
}

void application::run_compute()
{
	auto cmd_buf = SDL_AcquireGPUCommandBuffer(gpu.get());

	auto rw_uniform_binding = SDL_GPUStorageBufferReadWriteBinding{
		.buffer = scn.ter_cs_vtx_buffer.get(),
		.cycle  = false,
	};
	auto compute_pass = SDL_BeginGPUComputePass(cmd_buf, NULL, 0, &rw_uniform_binding, 1);
	{
		SDL_BindGPUComputePipeline(compute_pass, scn.comp_pipeline.get());

		auto sampler_binding = SDL_GPUTextureSamplerBinding{
			.texture = scn.terrain_heightmap.get(),
			.sampler = scn.terrain_sampler.get(),
		};
		SDL_BindGPUComputeSamplers(compute_pass, 0, &sampler_binding, 1);

		SDL_DispatchGPUCompute(compute_pass, scn.ter_cs_vtx_cnt / 1024, 1, 1);
	}
	SDL_EndGPUComputePass(compute_pass);
	SDL_SubmitGPUCommandBuffer(cmd_buf);
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

	auto depth_target = SDL_GPUDepthStencilTargetInfo{
		.texture          = scn.depth_texture.get(),
		.clear_depth      = 1.0f,
		.load_op          = SDL_GPU_LOADOP_CLEAR,
		.store_op         = SDL_GPU_STOREOP_STORE,
		.stencil_load_op  = SDL_GPU_LOADOP_CLEAR,
		.stencil_store_op = SDL_GPU_STOREOP_STORE,
		.cycle            = true,
		.clear_stencil    = 0,
	};

	auto render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, &depth_target);
	{
		SDL_BindGPUGraphicsPipeline(render_pass, scn.ter_cm_gfx_pl.get());

		auto vertex_bindings = std::array{
			SDL_GPUBufferBinding{
			  .buffer = scn.ter_cs_vtx_buffer.get(),
			  .offset = 0,
			},
		};
		SDL_BindGPUVertexBuffers(render_pass, 0, vertex_bindings.data(), static_cast<uint32_t>(vertex_bindings.size()));

		auto index_binding = SDL_GPUBufferBinding{
			.buffer = scn.ter_cs_idx_buffer.get(),
			.offset = 0,
		};
		SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

		auto sampler_binding = SDL_GPUTextureSamplerBinding{
			.texture = scn.terrain_heightmap.get(),
			.sampler = scn.terrain_sampler.get(),
		};
		SDL_BindGPUFragmentSamplers(render_pass, 0, &sampler_binding, 1);

		SDL_BindGPUVertexSamplers(render_pass, 0, &sampler_binding, 1);

		SDL_DrawGPUIndexedPrimitives(render_pass, scn.ter_cs_idx_cnt, 1, 0, 0, 0);
	}
	SDL_EndGPURenderPass(render_pass);

	SDL_SubmitGPUCommandBuffer(cmd_buf);
}
