#pragma once

#include "core/types.h"
#include "model.h"
#include "light.h"

namespace vkr::Render
{
	class Shader;
	class PipelineState;
}

namespace vkr::Graphics
{
	class ModelSceneObject;
	class Sky;
	class Terrain;
	class View;
	class ViewManager;
	class ViewRenderer;
	class ViewRenderData;
	class Scene
	{
	public:
		Scene();
		~Scene();

		View* CreateView();
		void DestroyView(View*);

		// Run per frame updates to the scene and its objects & views
		void Update();

		void AddModel(const Ref<Model>& model);
		void RemoveModel(const Ref<Model>& model);

		void AddLight(const Ref<LocalLight>& light);
		void RemoveLight(const Ref<LocalLight>& light);

		void AddDirectionalLight(const Ref<DirectionalLight>& light);
		void RemoveDirectionalLight(const Ref<DirectionalLight>& light);

		void AddTerrain(const Ref<Terrain>& terrain);
		void RemoveTerrain(const Ref<Terrain>& terrain);

	private:
		// Prepare render data for rendering for each view. 
		// I.e extract renderable information and store in list to be picked up by render tasks later
		void PrepareView(View* view);

		// For now only a simple list of scene objects, 
		// but later maybe a spatial partitioning structure of scene objects?
		// Quadtree, Octree, Grid?
		std::vector<Ref<ModelSceneObject>> m_Models;
		std::vector<Ref<LocalLight>> m_LocalLights;
		std::vector<Ref<DirectionalLight>> m_DirectionalLights;
		Ref<TerrainSceneObject> m_Terrain;

		struct PendingAction
		{
			enum class Type
			{
				Add,
				Remove,
			};
			enum class ObjectType
			{
				Model,
				LocalLight,
				DirectionalLight,
				Terrain
			};
			Type m_Type;
			ObjectType m_ObjectType;
			Ref<Model> m_Model;
			Ref<LocalLight> m_LocalLight;
			Ref<DirectionalLight> m_DirectionalLight;
			Ref<Terrain> m_Terrain;
		};

		std::mutex m_PendingActionsMutex;
		std::queue<PendingAction> m_PendingActions;

		UniquePtr<ViewManager> m_ViewManager;
		UniquePtr<ViewRenderer> m_ViewRenderer;

		UniquePtr<Sky> m_Sky;

		uint32_t m_MaterialHitGroupCounter = 0;
		std::unordered_map<Material*, uint32_t> m_MaterialToHitGroupId;
		std::vector<Render::RaytracingHitGroupDesc> m_HitGroupDescs;
		Ref<Render::Shader> m_TraceRaysDynamicShaderLib;
		Ref<Render::PipelineState> m_TraceRaysPipelineState;
	};
}