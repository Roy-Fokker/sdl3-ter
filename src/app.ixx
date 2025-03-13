module;

export module application;

import std;
import clock;
import types;

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
		}

		~application()
		{
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

	private:
		ter::clock clk{};

		window_ptr wnd = nullptr;
		gpu_ptr gpu    = nullptr;

		SDL_Event sdl_event{};
		bool quit = false;

		scene scn{};
	};
}
