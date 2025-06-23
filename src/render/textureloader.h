#pragma once
#include "core/types.h"
#include "render/texture.h"

namespace vkr::Render
{
	class TextureLoader
	{
	protected:
		enum class Type
		{
			DDS,
			PNG,
			TGA
		};

	public:
		TextureLoader(Type type) : m_Type(type) {}
		virtual bool LoadTexture(TextureDesc& outTextureDesc, TextureData& outTextureData, const std::filesystem::path& filepath) = 0;

	protected:
		const Type m_Type;
	};
}