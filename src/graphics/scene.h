#pragma once

#include "core/types.h"

namespace vkr::Graphics
{
	class SceneObject;
	class View;
	class Scene
	{
	public:
		Scene();
		~Scene();

		// These should be thread-safe
		Ref<View> CreateView();
		void DestroyView(const Ref<View>& view);
		// Add/Remove model/object?
		// Add/Remove light
		// Add/Remove this...
		// Add/Remove that...

		// Run per frame updates to the scene and its objects & views
		void Update();

		// Prepare render data for rendering for each view. 
		// I.e extract renderable information and store in list to be picked up by render tasks later
		void PrepareView(View& view);

		void AddObject(Ref<SceneObject> object) { m_SceneObjects.push_back(object); }

	private:
		// For now only a simple list of scene objects, 
		// but later maybe a spatial partitioning structure of scene objects?
		// Quadtree, Octree, Grid?
		std::vector<Ref<SceneObject>> m_SceneObjects;

		std::vector<Ref<View>> m_Views;
	};
}