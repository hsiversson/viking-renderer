#pragma once

#include "entity.h"
#include "component.h"
#include "world.h"
#include <vector>

namespace vkr::Game
{
	struct HierarchyComponent : public IComponent
	{
		Entity m_Parent;
		std::vector<Entity> m_Children;
		World* m_World;
	};
}