#include "world.h"

#include "graphics/scene.h"

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

}