#pragma once
#include "component.h"
#include "core/memory.h"
#include "graphics/model.h"

namespace vkr::Game
{
	struct ModelComponent : public IComponent
	{
		std::filesystem::path m_ModelFilePath;
		Ref<Graphics::Model> m_Model;
		bool m_CastShadows = true;
		bool m_ReceiveShadows = true;
	};
}