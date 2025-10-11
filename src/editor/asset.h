#pragma once

namespace vkr::Editor
{
	enum class AssetType : uint32_t
	{
		GLTF,
		Material,
		Texture,
	};

	struct AssetDragDropPayload
	{
		AssetType m_Type;
		char m_Path[252];
	};

	class Asset
	{

	};
}