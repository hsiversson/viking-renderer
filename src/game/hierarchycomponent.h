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

		void Serialize(Json& s) const
		{
			s["parent"] = m_Parent.GetHandle();

			Json children;
			for (auto& child : m_Children)
			{
				children.push_back(Json(child.GetHandle()));
			}
			s["children"] = children;
		}

		void Deserialize(const Json& s)
		{
			EntityRegistry& entityRegistry = m_World->GetEntityRegistry();

			const Json& parent = s.at("parent");
			const EntityHandle parentHandle = parent.get<EntityHandle>();
			m_Parent = Entity(parentHandle, &entityRegistry);

			const Json& children = s.at("children");
			for (const Json& child : children)
			{
				const EntityHandle handle = parent.get<EntityHandle>();
				m_Children.push_back(Entity(handle, &entityRegistry));
			}
		}
	};
}