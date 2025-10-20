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

		void Serialize(Json& s) const
		{
			s["modelFile"] = m_ModelFilePath;
			s["castShadows"] = m_CastShadows;
			s["receiveShadows"] = m_ReceiveShadows;
		}

		void Deserialize(const Json& s)
		{
			m_ModelFilePath = s.at("modelFile").get<std::string>();
			m_CastShadows = s.at("castShadows").get<bool>();
			m_ReceiveShadows = s.at("receiveShadows").get<bool>();
		}
	};
}