#pragma once
#include "entity.h"

namespace vkr::Graphics
{
	class Scene;
}

namespace vkr::Game
{
	class World
	{
	public:
		World();
		~World();

		void Update();

		Entity CreateEntity(const char* name = "Unnamed Entity");
		void DestroyEntity(const Entity& entity);

		Graphics::Scene* GetGraphicsScene() const;

		EntityRegistry& GetEntityRegistry();
		const EntityRegistry& GetEntityRegistry() const;

	private:
		EntityRegistry m_EntityRegistry;
		UniquePtr<Graphics::Scene> m_GraphicsScene;
	};
}