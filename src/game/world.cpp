#include "world.h"

#include "graphics/scene.h"
#include "hierarchycomponent.h"
#include "idcomponent.h"
#include "modelcomponent.h"
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
		Entity entity(m_EntityRegistry.create(), &m_EntityRegistry);

		IdComponent& idComponent = entity.AddComponent<IdComponent>();
		idComponent.m_Uid = entity.GetHandle();
		idComponent.m_Name = name;
		idComponent.m_Type = "Entity";

		Game::HierarchyComponent& hierarchy = entity.AddComponent<Game::HierarchyComponent>();
		hierarchy.m_World = this;
		entity.AddComponent<Game::TransformComponent>();

		return entity;
	}

	void World::DestroyEntity(const Entity& entity)
	{
		// TODO: Need to handle destruction flow better
		if (entity.HasComponent<Game::ModelComponent>())
		{
			const Game::ModelComponent& comp = entity.GetComponent<Game::ModelComponent>();
			m_GraphicsScene->RemoveModel(comp.m_Model);
		}

		m_EntityRegistry.destroy(entity);
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