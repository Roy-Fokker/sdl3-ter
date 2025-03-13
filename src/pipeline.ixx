module;

export module pipeline;

import std;
import io;
import types;

export namespace ter
{
	enum class shader_stage : uint8_t
	{
		vertex,
		fragment,
		compute,
	};

	struct shader_builder
	{
		byte_array shader_binary;
		shader_stage stage;
		uint32_t sampler_count         = 0;
		uint32_t uniform_buffer_count  = 0;
		uint32_t storage_uniform_count = 0;
		uint32_t storage_texture_count = 0;

		auto build(SDL_GPUDevice *gpu) const -> gfx_shader_ptr
		{
			assert(shader_binary.size() != 0 and "Shader binary is empty");

			auto to_sdl = [](shader_stage stage) -> SDL_GPUShaderStage {
				switch (stage)
				{
				case shader_stage::vertex:
					return SDL_GPU_SHADERSTAGE_VERTEX;
				case shader_stage::fragment:
					return SDL_GPU_SHADERSTAGE_FRAGMENT;
				}
				assert(false and "Unhandled shader stage");
				return {};
			};

			auto shader_info = SDL_GPUShaderCreateInfo{
				.code_size            = shader_binary.size(),
				.code                 = reinterpret_cast<const uint8_t *>(shader_binary.data()),
				.entrypoint           = "main",
				.format               = SHADER_FORMAT,
				.stage                = to_sdl(stage),
				.num_samplers         = sampler_count,
				.num_storage_textures = storage_texture_count,
				.num_storage_buffers  = storage_uniform_count,
				.num_uniform_buffers  = uniform_buffer_count,
			};

			auto shader = SDL_CreateGPUShader(gpu, &shader_info);
			assert(shader != nullptr and "Failed to create shader.");

			return { shader, { gpu } };
		}
	};

	enum class cull_mode : uint8_t
	{
		none,
		front_ccw,
		back_ccw,
		front_cw,
		back_cw,
	};

	struct gfx_pipeline_builder
	{
		gfx_shader_ptr vertex_shader;
		gfx_shader_ptr fragment_shader;

		std::span<const SDL_GPUVertexAttribute> vertex_attributes;
		std::span<const SDL_GPUVertexBufferDescription> vertex_buffer_descriptions;

		SDL_GPUTextureFormat color_format;
		bool enable_depth_test;

		cull_mode culling;

		auto build(SDL_GPUDevice *gpu) const -> gfx_pipeline_ptr
		{
			auto vertex_input_state = SDL_GPUVertexInputState{
				.vertex_buffer_descriptions = vertex_buffer_descriptions.data(),
				.num_vertex_buffers         = static_cast<uint32_t>(vertex_buffer_descriptions.size()),
				.vertex_attributes          = vertex_attributes.data(),
				.num_vertex_attributes      = static_cast<uint32_t>(vertex_attributes.size()),
			};

			auto rasterizer_state = [&]() -> SDL_GPURasterizerState {
				switch (culling)
				{
					using cm = cull_mode;
				case cm::none:
					return {
						.fill_mode = SDL_GPU_FILLMODE_FILL,
						.cull_mode = SDL_GPU_CULLMODE_NONE,
					};
				case cm::front_ccw:
					return {
						.fill_mode  = SDL_GPU_FILLMODE_FILL,
						.cull_mode  = SDL_GPU_CULLMODE_FRONT,
						.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
					};
				case cm::back_ccw:
					return {
						.fill_mode  = SDL_GPU_FILLMODE_FILL,
						.cull_mode  = SDL_GPU_CULLMODE_BACK,
						.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
					};
				case cm::front_cw:
					return {
						.fill_mode  = SDL_GPU_FILLMODE_FILL,
						.cull_mode  = SDL_GPU_CULLMODE_FRONT,
						.front_face = SDL_GPU_FRONTFACE_CLOCKWISE
					};
				case cm::back_cw:
					return {
						.fill_mode  = SDL_GPU_FILLMODE_FILL,
						.cull_mode  = SDL_GPU_CULLMODE_BACK,
						.front_face = SDL_GPU_FRONTFACE_CLOCKWISE
					};
				}
				assert(false and "Unhandled Cull Mode case.");
				return {};
			}();

			auto depth_stencil_state = SDL_GPUDepthStencilState{};
			if (enable_depth_test)
			{
				depth_stencil_state = SDL_GPUDepthStencilState{
					.compare_op          = SDL_GPU_COMPAREOP_LESS,
					.write_mask          = std::numeric_limits<uint8_t>::max(),
					.enable_depth_test   = true,
					.enable_depth_write  = true,
					.enable_stencil_test = false,
				};
			}

			auto blend_state = SDL_GPUColorTargetBlendState{
				.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
				.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				.color_blend_op        = SDL_GPU_BLENDOP_ADD,
				.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
				.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
				.alpha_blend_op        = SDL_GPU_BLENDOP_ADD,
				.enable_blend          = true,
			};

			auto color_targets = std::array{
				SDL_GPUColorTargetDescription{
				  .format      = color_format,
				  .blend_state = blend_state,
				},
			};

			auto target_info = SDL_GPUGraphicsPipelineTargetInfo{
				.color_target_descriptions = color_targets.data(),
				.num_color_targets         = static_cast<uint32_t>(color_targets.size()),
				.depth_stencil_format      = DEPTH_FORMAT,
				.has_depth_stencil_target  = enable_depth_test,
			};

			auto pipeline_info = SDL_GPUGraphicsPipelineCreateInfo{
				.vertex_shader       = vertex_shader.get(),
				.fragment_shader     = fragment_shader.get(),
				.vertex_input_state  = vertex_input_state,
				.primitive_type      = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
				.rasterizer_state    = rasterizer_state,
				.depth_stencil_state = depth_stencil_state,
				.target_info         = target_info,
			};
			auto pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);
			assert(pipeline != nullptr and "Failed to create pipeline.");

			return { pipeline, { gpu } };
		}
	};
}