module;

// DDS and KTX texture file loader
// DDSKTX_IMPLEMENT must be defined in exactly one translation unit
// library author says it's good practice to define it in the same file as the implementation
#define DDSKTX_IMPLEMENT
#include <dds-ktx.h>

export module io;

import std;

export namespace ter
{
	// Convience alias for a span of bytes
	using byte_array = std::vector<std::byte>;
	using byte_span  = std::span<const std::byte>;
	using byte_spans = std::span<byte_span>;

	auto as_byte_span(const auto &src) -> byte_span
	{
		return std::span{
			reinterpret_cast<const std::byte *>(&src),
			sizeof(src)
		};
	}

	auto as_byte_span(const std::ranges::contiguous_range auto &src) -> byte_span
	{
		auto src_span   = std::span{ src };      // convert to a span,
		auto byte_size  = src_span.size_bytes(); // so we can get size_bytes
		auto byte_start = reinterpret_cast<const std::byte *>(src.data());
		return { byte_start, byte_size };
	}

	auto offset_ptr(void *ptr, std::ptrdiff_t offset) -> void *
	{
		return reinterpret_cast<std::byte *>(ptr) + offset;
	}

	auto read_file(const std::filesystem::path &filename) -> byte_array
	{
		std::println("Reading file: {}", filename.string());

		auto file = std::ifstream(filename, std::ios::in | std::ios::binary);

		assert(file.good() and "failed to open file!");

		auto file_size = std::filesystem::file_size(filename);
		auto buffer    = byte_array(file_size);

		file.read(reinterpret_cast<char *>(buffer.data()), file_size);

		file.close();

		return buffer;
	}

	enum class image_format
	{
		invalid,
		Bc1,
		Bc2,
		Bc3,
		Bc4,
		Bc5,
		Bc6,
		Bc7,
	};

	struct image_t
	{
		struct header_t
		{
			image_format format;
			uint32_t width;
			uint32_t height;
			uint32_t depth;
			uint32_t layer_count;
			uint32_t mipmap_count;
			bool is_cubemap;
		};

		struct sub_image_t
		{
			uint32_t slice_index;
			uint32_t layer_index;
			uint32_t mipmap_index;
			uintptr_t offset;
			uint32_t width;
			uint32_t height;
		};

		header_t header;
		std::vector<sub_image_t> sub_images;
		byte_array data;
	};

	// Read DDS/KTX image file and return image_data object with file contents
	auto read_ddsktx_file(const std::filesystem::path &filename) -> image_t;
}

namespace
{
	auto to_format(ddsktx_format format) -> ter::image_format
	{
		using sf = ter::image_format;

		switch (format)
		{
		case DDSKTX_FORMAT_BC1: // DXT1
			return sf::Bc1;
		case DDSKTX_FORMAT_BC2: // DXT3
			return sf::Bc2;
		case DDSKTX_FORMAT_BC3: // DXT5
			return sf::Bc3;
		case DDSKTX_FORMAT_BC4: // ATI1
			return sf::Bc4;
		case DDSKTX_FORMAT_BC5: // ATI2
			return sf::Bc5;
		case DDSKTX_FORMAT_BC6H: // BC6H
			return sf::Bc6;
		case DDSKTX_FORMAT_BC7: // BC7
			return sf::Bc7;
		default:
			break;
		}

		assert(false and "Unmapped ddsktx_format, no conversion available");
		return sf::invalid;
	}
}

auto ter::read_ddsktx_file(const std::filesystem::path &filename) -> image_t
{
	// Load the file into memory
	auto image_file_data = read_file(filename);
	// get start pointer and size
	auto image_data_ptr  = static_cast<void *>(image_file_data.data());
	auto image_data_size = static_cast<int32_t>(image_file_data.size());

	auto image_data_header = ddsktx_texture_info{};
	auto image_parse_error = ddsktx_error{};

	// Parse DDS/KTX image file.
	auto result = ddsktx_parse(&image_data_header,
	                           image_data_ptr,
	                           image_data_size,
	                           &image_parse_error);
	assert(result == true and "Failed to parse image file data.");

	// convert ddsktx header info to image_header
	auto image_header = image_t::header_t{
		.format       = to_format(image_data_header.format),
		.width        = static_cast<uint32_t>(image_data_header.width),
		.height       = static_cast<uint32_t>(image_data_header.height),
		.depth        = static_cast<uint32_t>(image_data_header.depth),
		.layer_count  = static_cast<uint32_t>(image_data_header.num_layers),
		.mipmap_count = static_cast<uint32_t>(image_data_header.num_mips),
		.is_cubemap   = bool(image_data_header.flags & DDSKTX_TEXTURE_FLAG_CUBEMAP),
	};

	// alias views and ranges namespaces to save on visual noise
	namespace vw = std::views;
	namespace rg = std::ranges;

	auto slice_max = image_header.is_cubemap ? 6u : 1u;
	// make ranges that count from 0 upto but not including #
	auto slice_rng  = vw::iota(0u, slice_max);
	auto layer_rng  = vw::iota(0u, image_header.layer_count);
	auto mipmap_rng = vw::iota(0u, image_header.mipmap_count);

	// based on combination of layers and mipmaps, extract sub image information
	auto sub_img_rng = vw::cartesian_product(slice_rng, layer_rng, mipmap_rng)            // cartesian product will produce a pair-wise range
	                 | vw::transform([&](auto &&layer_mip_pair) -> image_t::sub_image_t { // transform will convert ddsktx_sub_data to image_data::sub_image
		auto [slice_idx, layer_idx, mip_idx] = layer_mip_pair;

		auto sub_data = ddsktx_sub_data{};
		ddsktx_get_sub(
			&image_data_header,
			&sub_data,
			image_data_ptr,
			image_data_size,
			layer_idx,
			slice_idx,
			mip_idx);

		auto sub_offset = uintptr_t(sub_data.buff) -
		                  uintptr_t(image_data_ptr) -
		                  uintptr_t(image_data_header.data_offset);

		return {
			.slice_index  = slice_idx,
			.layer_index  = layer_idx,
			.mipmap_index = mip_idx,
			.offset       = sub_offset,
			.width        = static_cast<uint32_t>(sub_data.width),
			.height       = static_cast<uint32_t>(sub_data.height),
		};
	});

	// evaluate above defined range to make it "concrete"
	auto sub_images = sub_img_rng | rg::to<std::vector>();

	// remove header information from image data
	auto start_itr = std::begin(image_file_data);
	image_file_data.erase(start_itr, start_itr + image_data_header.data_offset);
	image_file_data.shrink_to_fit();

	return {
		.header     = image_header,
		.sub_images = sub_images,
		.data       = image_file_data,
	};
}