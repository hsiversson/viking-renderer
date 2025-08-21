#pragma once
#include "component.h"

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
			ComponentType& newComponent = m_Components[m_EntityToIndex.at(handle)];
			newComponent.OnComponentAdded();
			return newComponent;
		}

		ComponentType* Get(EntityHandle handle)
		{
			if (Has(handle))
				return &(m_Components.at(m_EntityToIndex.at(handle)));
			else
				return nullptr;
		}

		const ComponentType* Get(EntityHandle handle) const
		{
			if (Has(handle))
				return &m_Components[m_EntityToIndex.at(handle)];
			else
				return nullptr;
		}

		void Remove(EntityHandle handle) override
		{
			if (!Has(handle))
				return;

			const size_t index = m_EntityToIndex.at(handle);
			const size_t lastIndex = m_Components.size() - 1;

			m_Components[index].OnComponentRemoved();
			std::swap(m_Components[index], m_Components[lastIndex]);
			std::swap(m_IndexToEntity[index], m_IndexToEntity[lastIndex]);

			m_EntityToIndex[m_IndexToEntity[index]] = index;

			m_Components.pop_back();
			m_IndexToEntity.pop_back();
			m_EntityToIndex.erase(handle);
		}

		std::vector<ComponentType>& View() { return m_Components; }
		const std::vector<ComponentType>& View() const { return m_Components; }

		std::vector<EntityHandle>& Entities() { return m_IndexToEntity; }
		const std::vector<EntityHandle>& Entities() const { return m_IndexToEntity; }

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
			return storage.Add(handle, ComponentType{ std::forward<Args>(args)... });
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
			if (auto storage = GetStorage<ComponentType>())
				return storage->Has(handle);
			else
				return false;
		}

		template<typename ComponentType>
		ComponentType* GetComponent(EntityHandle handle)
		{
			ComponentStorage<ComponentType>& storage = GetOrCreateStorage<ComponentType>();
			ComponentType* comp = storage.Get(handle);
			return comp;
		}

		template<typename ComponentType>
		const ComponentType* GetComponent(EntityHandle handle) const
		{
			if (auto storage = GetStorage<ComponentType>())
			{
				return storage->Get(handle);
			}
			else
				return nullptr;
		}

		template<typename ComponentType>
		std::vector<ComponentType>* ViewComponents()
		{
			ComponentStorage<ComponentType>& storage = GetOrCreateStorage<ComponentType>();
			std::vector<ComponentType>& view = storage.View();
			return &view;
		}

		template<typename ComponentType>
		const std::vector<ComponentType>* ViewComponents() const
		{
			if (auto storage = GetStorage<ComponentType>())
			{
				return &(storage->View());
			}
			else
				return nullptr;
		}

		template<typename ComponentType>
		std::vector<EntityHandle>* ViewEntities()
		{
			ComponentStorage<ComponentType>& storage = GetOrCreateStorage<ComponentType>();
			std::vector<EntityHandle>& view = storage.Entities();
			return &view;
		}

		template<typename ComponentType>
		const std::vector<EntityHandle>* ViewEntities() const
		{
			if (auto storage = GetStorage<ComponentType>())
			{
				return &(storage->Entities());
			}
			else
				return nullptr;
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

		template<typename ComponentType>
		const ComponentStorage<ComponentType>* GetStorage() const
		{
			std::type_index type = typeid(ComponentType);
			auto it = m_ComponentStorages.find(type);
			if (it == m_ComponentStorages.end())
				return nullptr;
			return static_cast<const ComponentStorage<ComponentType>*>(it->second.get());
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
		const ComponentType* GetComponent() const
		{
			VKR_ASSERT(m_Handle != EntityNullHandle);
			VKR_ASSERT(m_Registry);
			if (!HasComponent<ComponentType>())
				return nullptr;

			return m_Registry->GetComponent<ComponentType>(m_Handle);
		}

		template<typename ComponentType>
		ComponentType* GetComponent()
		{
			VKR_ASSERT(m_Handle != EntityNullHandle);
			VKR_ASSERT(m_Registry);
			if (!HasComponent<ComponentType>())
				return nullptr;

			return m_Registry->GetComponent<ComponentType>(m_Handle);
		}

		template<typename ComponentType, typename... Args>
		ComponentType& AddComponent(Args&&... args)
		{
			if (HasComponent<ComponentType>())
			{
				return *(GetComponent<ComponentType>());
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