#pragma once

#include "core/types.h"

namespace vkr::Render
{
	class Shader;
	class PipelineState;
}

namespace vkr::Graphics
{
	class SceneObject; 
	class ViewManager;
	class ViewRenderer;
	class View;
	class Scene
	{
	public:
		Scene();
		~Scene();

		// These should be thread-safe
		View* CreateView();
		void DestroyView(View*);
		// Add/Remove model/object?
		// Add/Remove light
		// Add/Remove this...
		// Add/Remove that...

		// Run per frame updates to the scene and its objects & views
		void Update();

		void AddObject(Ref<SceneObject> object) 
		{
			m_SceneObjects.push_back(object); 
			m_HasChanges = true;
		}

	private:
		// Prepare render data for rendering for each view. 
		// I.e extract renderable information and store in list to be picked up by render tasks later
		void PrepareView(View& view);

		// For now only a simple list of scene objects, 
		// but later maybe a spatial partitioning structure of scene objects?
		// Quadtree, Octree, Grid?
		std::vector<Ref<SceneObject>> m_SceneObjects;

		UniquePtr<ViewManager> m_ViewManager;
		UniquePtr<ViewRenderer> m_ViewRenderer;

		Ref<Render::Shader> m_TraceRaysDynamicShaderLib;
		Ref<Render::PipelineState> m_TraceRaysPipelineState;
		bool m_HasChanges;
	};
}