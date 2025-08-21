#pragma once

#include "entity.h"
#include "component.h"
#include <vector>

namespace vkr::Game
{
	struct HierarchyComponent : public IComponent
	{
		Entity m_Parent;
		std::vector<Entity> m_Children;
	};
}