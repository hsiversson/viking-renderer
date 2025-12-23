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
	};
}