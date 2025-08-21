#include "entity.h"
#include "hierarchycomponent.h"

namespace vkr::Game
{
	EntityHandle EntityRegistry::CreateEntity()
	{
		return m_NextEntityHandle++;
	}

	void EntityRegistry::DestroyEntity(EntityHandle handle)
	{
		for (auto& [type, storage] : m_ComponentStorages)
		{
			storage->Remove(handle);
		}
	}

	Entity::Entity()
		: m_Handle(EntityNullHandle)
		, m_Registry(nullptr)
	{
	}

	Entity::Entity(EntityHandle handle, EntityRegistry* registry)
		: m_Handle(handle)
		, m_Registry(registry)
	{
	}

	const Entity& Entity::GetParent() const
	{
		return GetComponent<HierarchyComponent>()->m_Parent;
	}

	const std::vector<Entity>& Entity::GetChildren() const
	{
		return GetComponent<HierarchyComponent>()->m_Children;
	}

	void Entity::AddChild(Entity child)
	{
		HierarchyComponent* comp = GetComponent<HierarchyComponent>();
		comp->m_Children.push_back(child);

		HierarchyComponent* childComp = child.GetComponent<HierarchyComponent>();
		childComp->m_Parent = *this;
	}

	void Entity::RemoveChild(Entity child)
	{
		HierarchyComponent* comp = GetComponent<HierarchyComponent>();
		auto it = std::find(comp->m_Children.begin(), comp->m_Children.end(), child);
		if (it != comp->m_Children.end())
		{
			comp->m_Children.erase(it);
		}

		HierarchyComponent* childComp = child.GetComponent<HierarchyComponent>();
		childComp->m_Parent = Entity();
	}

	bool Entity::IsValid() const
	{
		return m_Handle != EntityNullHandle;
	}

	Entity::operator bool() const
	{
		return IsValid();
	}

	Entity::operator EntityHandle() const
	{
		return m_Handle;
	}

	EntityHandle Entity::GetHandle() const
	{
		return m_Handle;
	}

	bool Entity::operator==(const Entity& other) const
	{
		return (m_Handle == other.m_Handle) && (m_Registry == other.m_Registry);
	}

	bool Entity::operator!=(const Entity& other) const
	{
		return !(*this == other);
	}
}