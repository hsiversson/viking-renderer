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

		}
	};
}