#pragma once
#include "component.h"
#include "core/types.h"
#include "graphics/terrain.h"

namespace vkr::Game
{
	struct TerrainComponent : public IComponent
	{
		std::filesystem::path m_HeightmapFilePath;
		Ref<Graphics::Terrain> m_Terrain;

		PROPERTY(editable, min = 1, max = 10, default = 1)
			int m_ClipmapLevels = 1;

	};
}