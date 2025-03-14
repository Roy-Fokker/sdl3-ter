module;

export module types;

import std;
import io;

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
	using free_gpu_buffer   = gpu_deleter<SDL_ReleaseGPUBuffer>;
	using gpu_buffer_ptr    = std::unique_ptr<SDL_GPUBuffer, free_gpu_buffer>;
	using free_gpu_texture  = gpu_deleter<SDL_ReleaseGPUTexture>;
	using gpu_texture_ptr   = std::unique_ptr<SDL_GPUTexture, free_gpu_texture>;
	using free_gpu_sampler  = gpu_deleter<SDL_ReleaseGPUSampler>;
	using gfx_sampler_ptr   = std::unique_ptr<SDL_GPUSampler, free_gpu_sampler>;

	auto to_sdl(image_format format) -> SDL_GPUTextureFormat
	{
		switch (format)
		{
		case image_format::Bc1:
			return SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM;
		case image_format::Bc2:
			return SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM;
		case image_format::Bc3:
			return SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM;
		case image_format::Bc4:
			return SDL_GPU_TEXTUREFORMAT_BC4_R_UNORM;
		case image_format::Bc5:
			return SDL_GPU_TEXTUREFORMAT_BC5_RG_UNORM;
		case image_format::Bc6:
			return SDL_GPU_TEXTUREFORMAT_BC6H_RGB_FLOAT;
		case image_format::Bc7:
			return SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM;
		}
		return SDL_GPU_TEXTUREFORMAT_INVALID;
	}
}