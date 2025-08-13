#include "entity.h"

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
		: m_Handle()
		, m_Registry(nullptr)
	{
	}

	Entity::Entity(EntityHandle handle, EntityRegistry* registry)
		: m_Handle(handle)
		, m_Registry(registry)
	{
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

	bool Entity::operator==(const Entity& other) const
	{
		return (m_Handle == other.m_Handle) && (m_Registry == other.m_Registry);
	}

	bool Entity::operator!=(const Entity& other) const
	{
		return !(*this == other);
	}
}