#pragma once

namespace vkr::Game
{
	class EntityRegistry;
	class World;

	using EntityHandle = uint64_t;
	static constexpr EntityHandle EntityNullHandle = EntityHandle(-1);

	class IComponentStorage
	{
	public:
		virtual ~IComponentStorage() = default;
		virtual void Remove(EntityHandle) = 0;
	};

	template<typename ComponentType>
	class ComponentStorage : public IComponentStorage
	{
	public:
		bool Has(EntityHandle handle) const
		{
			return m_EntityToIndex.contains(handle);
		}

		ComponentType& Add(EntityHandle handle, const ComponentType& component = ComponentType{})
		{
			if (!Has(handle))
			{
				size_t index = m_Components.size();
				m_EntityToIndex[handle] = index;
				m_Components.push_back(component);
				m_IndexToEntity.push_back(handle);
			}
			return m_Components[m_EntityToIndex.at(handle)];
		}

		ComponentType& Get(EntityHandle handle)
		{
			VKR_ASSERT(Has(handle), "Entity does not have component, make sure to add it before getting.");
			return m_Components[m_EntityToIndex.at(handle)];
		}

		void Remove(EntityHandle handle) override
		{
			if (!Has(handle))
				return;

			const size_t index = m_EntityToIndex.at(handle);
			const size_t lastIndex = m_Components.size() - 1;

			std::swap(m_Components[index], m_Components[lastIndex]);
			std::swap(m_IndexToEntity[index], m_IndexToEntity[lastIndex]);

			m_EntityToIndex[m_IndexToEntity[index]] = index;

			m_Components.pop_back();
			m_IndexToEntity.pop_back();
			m_EntityToIndex.erase(handle);
		}

	private:
		std::vector<ComponentType> m_Components;
		std::vector<EntityHandle> m_IndexToEntity;
		std::unordered_map<EntityHandle, size_t> m_EntityToIndex;
	};

	class EntityRegistry
	{
	public:
		EntityHandle CreateEntity();
		void DestroyEntity(EntityHandle handle);

		template<typename ComponentType, typename... Args>
		ComponentType& AddComponent(EntityHandle handle, Args&&... args)
		{
			auto& storage = GetOrCreateStorage<ComponentType>();
			return storage.Add(handle, ComponentType{std::forward<Args>(args)...});
		}

		template<typename ComponentType>
		void RemoveComponent(EntityHandle handle)
		{
			auto& storage = GetOrCreateStorage<ComponentType>();
			return storage.Remove(handle);
		}

		template<typename ComponentType>
		bool HasComponent(EntityHandle handle) const
		{
			auto& storage = GetOrCreateStorage<ComponentType>();
			return storage.Has(handle);
		}

		template<typename ComponentType>
		ComponentType& GetComponent(EntityHandle handle) const
		{
			auto& storage = GetOrCreateStorage<ComponentType>();
			return storage.Get(handle);
		}

	private:
		template<typename ComponentType>
		ComponentStorage<ComponentType>& GetOrCreateStorage()
		{
			std::type_index type = typeid(ComponentType);
			auto it = m_ComponentStorages.find(type);
			if (it == m_ComponentStorages.end()) 
			{
				auto pool = MakeUnique<ComponentStorage<ComponentType>>();
				auto ptr = pool.get();
				m_ComponentStorages[type] = std::move(pool);
				return *ptr;
			}
			return *static_cast<ComponentStorage<ComponentType>*>(m_ComponentStorages[type].get());
		}

		EntityHandle m_NextEntityHandle = 1;
		std::unordered_map<std::type_index, UniquePtr<IComponentStorage>> m_ComponentStorages;
	};

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
			return m_Registry->HasComponent<ComponentType>(m_Handle);
		}

		template<typename ComponentType>
		ComponentType& GetComponent() const
		{
			VKR_ASSERT(m_Handle != EntityNullHandle);
			VKR_ASSERT(m_Registry);
			return m_Registry->GetComponent<ComponentType>(m_Handle);
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
			return m_Registry->AddComponent<ComponentType>(m_Handle, std::forward<Args>(args)...);
		}

		template<typename ComponentType>
		void RemoveComponent() const
		{
			VKR_ASSERT(m_Handle != EntityNullHandle);
			VKR_ASSERT(m_Registry);
			return m_Registry->RemoveComponent<ComponentType>(m_Handle);
		}

		bool IsValid() const;
		operator bool() const;
		operator EntityHandle() const;

		bool operator==(const Entity& other) const;
		bool operator!=(const Entity& other) const;

	private:
		EntityHandle m_Handle;
		EntityRegistry* m_Registry;
	};
}