#include "world.h"

#include "graphics/scene.h"
#include "hierarchycomponent.h"
#include "idcomponent.h"
#include "transformcomponent.h"

namespace vkr::Game
{

	World::World()
		: m_GraphicsScene(MakeUnique<Graphics::Scene>())
	{

	}

	World::~World()
	{

	}

	void World::Update()
	{
		m_GraphicsScene->Update();
	}

	Entity World::CreateEntity(const char* name /*= "Unnamed Entity"*/)
	{
		Entity newEntity = Entity(m_EntityRegistry.CreateEntity(), &m_EntityRegistry);
		IdComponent& idComponent = newEntity.AddComponent<IdComponent>();
		idComponent.m_Uid = newEntity.GetHandle();
		idComponent.m_Name = name;
		idComponent.m_Type = "Entity";

		newEntity.AddComponent<Game::HierarchyComponent>();
		newEntity.AddComponent<Game::TransformComponent>();

		return newEntity;
	}

	void World::DestroyEntity(const Entity& entity)
	{
		m_EntityRegistry.DestroyEntity(entity);
	}

	Graphics::Scene* World::GetGraphicsScene() const
	{
		return m_GraphicsScene.get();
	}

	EntityRegistry& World::GetEntityRegistry()
	{
		return m_EntityRegistry;
	}

	const EntityRegistry& World::GetEntityRegistry() const
	{
		return m_EntityRegistry;
	}

}