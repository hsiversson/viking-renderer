#pragma once
#include "component.h"

#define ENTT_ID_TYPE uint64_t
#include "entt/entt.hpp"

namespace vkr::Game
{
	using EntityHandle = entt::entity;
	using EntityNullHandleType = entt::null_t;
	using EntityRegistry = entt::registry;

	static constexpr EntityNullHandleType EntityNullHandle = entt::null;

	class Entity
	{
	public:
		Entity();
		Entity(EntityHandle handle, EntityRegistry* registry);

		template<typename ComponentType>
		bool HasComponent() const
		{
			VKR_ASSERT(m_Handle != EntityNullHandle);
			VKR_ASSERT(m_Registry);
			return m_Registry->all_of<ComponentType>(m_Handle);
		}

		template<typename ComponentType>
		const ComponentType& GetComponent() const
		{
			VKR_ASSERT(m_Handle != EntityNullHandle);
			VKR_ASSERT(m_Registry);
			return m_Registry->get<ComponentType>(m_Handle);
		}

		template<typename ComponentType>
		ComponentType& GetComponent()
		{
			VKR_ASSERT(m_Handle != EntityNullHandle);
			VKR_ASSERT(m_Registry);
			return m_Registry->get<ComponentType>(m_Handle);
		}

		template<typename ComponentType, typename... Args>
		ComponentType& AddComponent(Args&&... args)
		{
			if (HasComponent<ComponentType>())
			{
				return GetComponent<ComponentType>();
			}

			VKR_ASSERT(m_Handle != EntityNullHandle);
			VKR_ASSERT(m_Registry);
			return m_Registry->emplace<ComponentType>(m_Handle, std::forward<Args>(args)...);
		}

		template<typename ComponentType>
		void RemoveComponent() const
		{
			VKR_ASSERT(m_Handle != EntityNullHandle);
			VKR_ASSERT(m_Registry);
			return m_Registry->remove<ComponentType>(m_Handle);
		}

		template<typename Fn>
		void ForEachComponent(Fn&& func)
		{
			for (auto&& curr : m_Registry->storage())
			{
				entt::id_type cid = curr.first;
				entt::sparse_set& storage = curr.second;

				if (storage.contains(m_Handle))
				{
					func(storage.value(m_Handle));
				}
			}
		}

		const Entity& GetParent() const;
		const std::vector<Entity>& GetChildren() const;
		void AddChild(Entity child);
		void RemoveChild(Entity child);

		bool IsValid() const;
		operator bool() const;
		operator EntityHandle() const;
		EntityHandle GetHandle() const;

		bool operator==(const Entity& other) const;
		bool operator!=(const Entity& other) const;

	private:
		EntityHandle m_Handle;
		EntityRegistry* m_Registry;
	};
}