module;

#include <cassert>
#include <SDL3/SDL.h>

export module application;

import std;
import clock;

using namespace std::literals;

namespace
{
	constexpr auto IS_DEBUG = bool{ _DEBUG };

	// Deleter template, for use with SDL objects.
	// Allows use of SDL Objects with C++'s smart pointers, using SDL's destroy function
	template <auto fn>
	struct sdl_deleter
	{
		constexpr void operator()(auto *arg)
		{
			fn(arg);
		}
	};
	// Define SDL types with std::unique_ptr and custom deleter;
	using gpu_ptr    = std::unique_ptr<SDL_GPUDevice, sdl_deleter<SDL_DestroyGPUDevice>>;
	using window_ptr = std::unique_ptr<SDL_Window, sdl_deleter<SDL_DestroyWindow>>;

	template <auto fn>
	struct gpu_deleter
	{
		SDL_GPUDevice *gpu = nullptr;
		constexpr void operator()(auto *arg)
		{
			fn(gpu, arg);
		}
	};

}

export namespace ter
{
	class application
	{
	public:
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
			auto gdev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, IS_DEBUG, NULL);
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
		window_ptr wnd = nullptr;
		gpu_ptr gpu    = nullptr;

		bool quit = false;
		SDL_Event sdl_event{};

		ter::clock clk{};
	};
}
