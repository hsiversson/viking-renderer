#pragma once

#include "entity.h"
#include "component.h"
#include <string>

namespace vkr::Game
{
	struct IdComponent : public IComponent
	{
		EntityHandle m_Uid;
		std::string m_Name;
		std::string m_Type;

		void Serialize(Json& s) const
		{
			s["uid"] = m_Uid;
			s["name"] = m_Name;
			s["type"] = m_Type;
		}

		void Deserialize(const Json& s)
		{
			m_Uid = s.at("uid");
			m_Name = s.at("name");
			m_Type = s.at("type");
		}
	};
}