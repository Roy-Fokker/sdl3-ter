module;

export module application;

import std;
import clock;
import io;

import types;
import pipeline;

using namespace std::literals;

namespace
{
	using namespace ter;

	constexpr auto IS_DEBUG = bool{ _DEBUG };

	struct scene
	{
		SDL_FColor clear_color;

		gfx_pipeline_ptr pipeline;
	};

	struct vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
	};

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
			constexpr auto width  = 800;
			constexpr auto height = 600;
			constexpr auto title  = "SDL3 Terrain"sv;

			auto window = SDL_CreateWindow(title.data(), width, height, NULL);
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

			make_pipeline();
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

		auto get_swapchain_texture(SDL_Window *win, SDL_GPUCommandBuffer *cmd_buf) -> SDL_GPUTexture *
		{
			auto sc_tex = (SDL_GPUTexture *)nullptr;

			auto res = SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, win, &sc_tex, NULL, NULL);
			assert(res == true and "Wait and acquire GPU swapchain texture failed.");
			assert(sc_tex != nullptr and "Swapchain texture is null. Is window minimized?");

			return sc_tex;
		}

		void draw()
		{
			auto device = gpu.get();
			auto window = wnd.get();

			auto cmd_buf = SDL_AcquireGPUCommandBuffer(device);
			assert(cmd_buf != nullptr and "Failed to acquire command buffer.");

			auto sc_img = get_swapchain_texture(window, cmd_buf);

			auto color_target = SDL_GPUColorTargetInfo{
				.texture     = sc_img,
				.clear_color = scn.clear_color,
				.load_op     = SDL_GPU_LOADOP_CLEAR,
				.store_op    = SDL_GPU_STOREOP_STORE,
			};

			// auto depth_target = SDL_GPUDepthStencilTargetInfo{
			// 	.texture          = scn.depth_texture.get(),
			// 	.clear_depth      = 1.0f,
			// 	.load_op          = SDL_GPU_LOADOP_CLEAR,
			// 	.store_op         = SDL_GPU_STOREOP_STORE,
			// 	.stencil_load_op  = SDL_GPU_LOADOP_CLEAR,
			// 	.stencil_store_op = SDL_GPU_STOREOP_STORE,
			// 	.cycle            = true,
			// 	.clear_stencil    = 0,
			// };

			auto render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, /*&depth_target*/ nullptr);
			{
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
	};
}
