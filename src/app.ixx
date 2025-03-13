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

namespace
{
	using namespace ter;

	constexpr auto IS_DEBUG   = bool{ _DEBUG };
	constexpr auto WND_WIDTH  = 1280;
	constexpr auto WND_HEIGHT = WND_WIDTH * 9 / 16;
	constexpr auto FOV_ANGLE  = 90.f;

	struct scene
	{
		SDL_FColor clear_color;
		struct uniform_data
		{
			glm::mat4 projection;
			glm::mat4 view;
		} proj_view;

		gfx_pipeline_ptr pipeline;
		gpu_buffer_ptr vertex_buffer;
		gpu_buffer_ptr index_buffer;
		uint32_t vertex_count;
		uint32_t index_count;
	};

	struct vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
	};
}

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
		void make_window()
		{
			constexpr auto title = "SDL3 Terrain"sv;

			auto window = SDL_CreateWindow(title.data(), WND_WIDTH, WND_HEIGHT, NULL);
			assert(window != nullptr and "Window could not be created.");

			wnd = window_ptr{ window };
		}

		void make_gpu()
		{
			auto gdev = SDL_CreateGPUDevice(SHADER_FORMAT, IS_DEBUG, NULL);
			assert(gdev != nullptr and "GPU device could not be created.");

			std::println("GPU API Name: {}", SDL_GetGPUDeviceDriver(gdev));

			gpu = gpu_ptr{ gdev };
		}

		void handle_sdl_events()
		{
			while (SDL_PollEvent(&sdl_event))
			{
				switch (sdl_event.type)
				{
				case SDL_EVENT_QUIT:
					quit = true;
					return;
				case SDL_EVENT_KEY_DOWN:
					quit = true;
					return;
				default:
					break;
				}
			}
		}

		void initialize_scene()
		{
			scn.clear_color = { 0.2f, 0.4f, 0.4f, 1.0f };

			make_perspective();
			place_camera();

			make_pipeline();

			make_mesh();
		}

		void make_perspective()
		{
			float fovy         = glm::radians(FOV_ANGLE);
			float aspect_ratio = static_cast<float>(WND_WIDTH) / WND_HEIGHT;
			float near_plane   = 0.1f;
			float far_plane    = 100.f;

			scn.proj_view.projection = glm::perspective(fovy, aspect_ratio, near_plane, far_plane);
		}

		void place_camera()
		{
			cam.lookat(glm::vec3{ 0.0f, 1.0f, -1.0f },
			           glm::vec3{ 0.0f, 0.0f, 0.0f },
			           glm::vec3{ 0.0f, 1.0f, 0.0f });
			scn.proj_view.view = cam.get_view();
		}

		void make_pipeline()
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
			scn.pipeline = pl.build(gpu.get());
		}

		void make_mesh()
		{
			auto vertices = std::vector<vertex>{};
			auto indices  = std::vector<uint32_t>{};

			{ // Make Mesh
				auto x = 0.5f, y = 0.0f, z = 0.5f;

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
				for (auto i : vw::iota(-5, 5))
				{
					for (auto j : vw::iota(-5, 5))
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
							return idx + i_offset;
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

		void draw()
		{
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
				SDL_BindGPUGraphicsPipeline(render_pass, scn.pipeline.get());
			}
			SDL_EndGPURenderPass(render_pass);

			SDL_SubmitGPUCommandBuffer(cmd_buf);
		}

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
