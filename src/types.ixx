module;

export module types;

import std;

export namespace ter
{
	constexpr auto SHADER_FORMAT = SDL_GPU_SHADERFORMAT_DXIL;
	constexpr auto DEPTH_FORMAT  = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;

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
	using free_gfx_pipeline = gpu_deleter<SDL_ReleaseGPUGraphicsPipeline>;
	using gfx_pipeline_ptr  = std::unique_ptr<SDL_GPUGraphicsPipeline, free_gfx_pipeline>;
	using free_gfx_shader   = gpu_deleter<SDL_ReleaseGPUShader>;
	using gfx_shader_ptr    = std::unique_ptr<SDL_GPUShader, free_gfx_shader>;

}