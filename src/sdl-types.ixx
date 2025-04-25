module;

export module sdl:types;

import std;
import io;

export namespace ter
{
	constexpr auto IS_DEBUG = bool{
#ifdef DEBUG
		true
#endif
	};

	constexpr auto MAX_ANISOTROPY = float{ 16 };

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
	using window_ptr = std::unique_ptr<SDL_Window, sdl_deleter<SDL_DestroyWindow>>;

	// Special deleter for gpu.
	// it will release window on destruction
	struct gpu_window_deleter
	{
		SDL_Window *window;
		constexpr void operator()(auto *gpu)
		{
			SDL_ReleaseWindowFromGPUDevice(gpu, window);
			SDL_DestroyGPUDevice(gpu);
		}
	};
	// Define GPU type with std::unique_ptr and custom deleter
	using gpu_ptr = std::unique_ptr<SDL_GPUDevice, gpu_window_deleter>;

	template <auto fn>
	struct gpu_deleter
	{
		SDL_GPUDevice *gpu = nullptr;
		constexpr void operator()(auto *arg)
		{
			fn(gpu, arg);
		}
	};
	using free_gfx_pipeline  = gpu_deleter<SDL_ReleaseGPUGraphicsPipeline>;
	using gfx_pipeline_ptr   = std::unique_ptr<SDL_GPUGraphicsPipeline, free_gfx_pipeline>;
	using free_comp_pipeline = gpu_deleter<SDL_ReleaseGPUComputePipeline>;
	using comp_pipeline_ptr  = std::unique_ptr<SDL_GPUComputePipeline, free_comp_pipeline>;
	using free_gfx_shader    = gpu_deleter<SDL_ReleaseGPUShader>;
	using gfx_shader_ptr     = std::unique_ptr<SDL_GPUShader, free_gfx_shader>;
	using free_gpu_buffer    = gpu_deleter<SDL_ReleaseGPUBuffer>;
	using gpu_buffer_ptr     = std::unique_ptr<SDL_GPUBuffer, free_gpu_buffer>;
	using free_gpu_texture   = gpu_deleter<SDL_ReleaseGPUTexture>;
	using gpu_texture_ptr    = std::unique_ptr<SDL_GPUTexture, free_gpu_texture>;
	using free_gpu_sampler   = gpu_deleter<SDL_ReleaseGPUSampler>;
	using gfx_sampler_ptr    = std::unique_ptr<SDL_GPUSampler, free_gpu_sampler>;

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
		assert(false and "Unhandled image format.");
		return SDL_GPU_TEXTUREFORMAT_INVALID;
	}

	enum class sampler_type : uint8_t
	{
		point_clamp,
		point_wrap,
		linear_clamp,
		linear_wrap,
		anisotropic_clamp,
		anisotropic_wrap,
	};

	auto to_sdl(sampler_type type) -> SDL_GPUSamplerCreateInfo
	{
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
		assert(false and "Unhandled sampler type");
		return {};
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

	auto make_gpu_texture(SDL_GPUDevice *gpu, const SDL_GPUTextureCreateInfo &texture_info, std::string_view debug_name) -> gpu_texture_ptr
	{
		auto texture = SDL_CreateGPUTexture(gpu, &texture_info);
		assert(texture != nullptr and "Failed to create gpu texture.");

		if (IS_DEBUG and debug_name.size() > 0)
		{
			SDL_SetGPUTextureName(gpu, texture, debug_name.data());
		}

		return { texture, { gpu } };
	}

	auto make_gpu_texture(SDL_GPUDevice *gpu, const ter::image_t::header_t img_hdr, std::string_view debug_name) -> gpu_texture_ptr
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

		return make_gpu_texture(gpu, texture_info, debug_name);
	}

	auto make_gpu_sampler(SDL_GPUDevice *gpu, sampler_type type) -> gfx_sampler_ptr
	{
		auto sampler_info = to_sdl(type);

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